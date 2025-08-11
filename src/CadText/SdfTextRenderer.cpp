#include <CadText/SdfTextRenderer.h>

namespace CadText {

const uint32_t SdfTextRenderer::vertexShaderSpirv[] = {
#include "shaders/sdf.vert.spv"
};

const uint32_t SdfTextRenderer::fragmentShaderSpirv[] = {
#include "shaders/sdf.frag.spv"
};

SdfTextRenderer::SdfTextRenderer(CadR::Renderer &renderer, vk::RenderPass renderPass, vk::Extent2D extent)
    : CadText::SdfTextRenderer{renderer} {
    this->setViewportSize(extent.width, extent.height);
    this->createPipelines(renderPass, extent);
}

SdfTextRenderer::SdfTextRenderer(CadR::Renderer &renderer)
    : CadText::TextRenderer{renderer}, _boundingBoxStateSet{renderer}, _sampler{renderer} {
    this->createShaderModules();

    this->_rootStateSet.childList.append(this->_boundingBoxStateSet);

    this->_boundingBoxStateSet.recordCallList.push_back(
        [&](CadR::StateSet &stateSet, vk::CommandBuffer commandBuffer) -> void {
            this->_renderer->device().cmdPushConstants(
                commandBuffer,                                                          // commandBuffer
                this->_pipelines[0].layout(),                                           // pipelineLayout
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,  // stageFlags
                0,                                                                      // offset
                2 * sizeof(uint64_t),                                                   // size
                std::array<uint64_t, 2>{
                    this->_renderer->drawablePayloadDeviceAddress(),  // payloadBufferPtr
                    this->_sceneData                                  // sceneDataPtr
                }
                    .data()  // pValues
            );
        });

    this->_sampler.create(vk::SamplerCreateInfo{
        vk::SamplerCreateFlags{},          // flags
        vk::Filter::eLinear,               // magFilter
        vk::Filter::eLinear,               // minFilter
        vk::SamplerMipmapMode::eLinear,    // mipmapMode
        vk::SamplerAddressMode::eRepeat,   // addressModeU
        vk::SamplerAddressMode::eRepeat,   // addressModeV
        vk::SamplerAddressMode::eRepeat,   // addressModeW
        0.f,                               // mipLodBias
        VK_FALSE,                          // anisotropyEnable
        0.f,                               // maxAnisotropy
        VK_FALSE,                          // compareEnable
        vk::CompareOp::eNever,             // compareOp
        0.f,                               // minLod
        0.f,                               // maxLod
        vk::BorderColor::eIntOpaqueBlack,  // borderColor
        VK_FALSE                           // unnormalizedCoordinates
    });
}

SdfTextRenderer::~SdfTextRenderer() {
    this->destroyPipelines();

    // Destroy shader modules
    this->_renderer->device().destroyShaderModule(this->_vertexShaderModule);
    this->_renderer->device().destroyShaderModule(this->_fragmentShaderModule);
}

void SdfTextRenderer::update() {
    this->_drawables.clear();
    this->_geometries.clear();
    this->_geometryOffsets.clear();

    for (std::shared_ptr<vft::TextBlock> textBlock : this->_textBlocks) {
        for (const vft::Character &character : textBlock->getCharacters()) {
            vft::GlyphKey key{character.getFont()->getFontFamily(), character.getGlyphId(), 0};

            // Create geometry for glyph if neccessary
            if (!this->_geometryOffsets.contains(key)) {
                // Insert glyph into cache
                if (!this->_cache->exists(key)) {
                    this->_cache->setGlyph(key,
                                           this->_tessellator->composeGlyph(character.getGlyphId(), character.getFont(),
                                                                            character.getFontSize()));
                }

                // Skip glyphs with no geometry
                const vft::Glyph &glyph = this->_cache->getGlyph(key);
                if (glyph.mesh.getIndexCount(vft::SdfTessellator::GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX) == 0) {
                    continue;
                }

                CadR::Geometry &geometry = this->_geometries.emplace_back(*this->_renderer);
                this->_geometryOffsets.insert({key, static_cast<unsigned int>(this->_geometryOffsets.size())});

                // Get uv coordinnates from font atlas
                if (!this->_fontAtlases.contains(character.getFont()->getFontFamily())) {
                    throw std::runtime_error("SdfTextRenderer::update(): Font atlas for font " +
                                             character.getFont()->getFontFamily() + " was not found");
                }

                vft::FontAtlas::GlyphInfo glyphInfo =
                    this->_fontAtlases.at(character.getFont()->getFontFamily()).getGlyph(character.getGlyphId());
                glm::vec2 uvTopLeft = glyphInfo.uvTopLeft;
                glm::vec2 uvBottomRight = glyphInfo.uvBottomRight;
                glm::vec2 uvTopRight{uvBottomRight.x, uvTopLeft.y};
                glm::vec2 uvBottomLeft{uvTopLeft.x, uvBottomRight.y};

                // Calculate bounding box vertices and uvs
                std::vector<Vertex> vertices;
                vertices.push_back(Vertex{glyph.mesh.getVertices().at(0), uvBottomLeft});
                vertices.push_back(Vertex{glyph.mesh.getVertices().at(1), uvTopLeft});
                vertices.push_back(Vertex{glyph.mesh.getVertices().at(2), uvTopRight});
                vertices.push_back(Vertex{glyph.mesh.getVertices().at(3), uvBottomRight});

                geometry.uploadVertexData(vertices.data(), vertices.size() * sizeof(vertices[0]));
                geometry.uploadIndexData(
                    glyph.mesh.getIndices(vft::SdfTessellator::GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX).data(),
                    glyph.mesh.getIndexCount(vft::SdfTessellator::GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX) *
                        sizeof(uint32_t));

                CadR::PrimitiveSet primitiveSet{
                    glyph.mesh.getIndexCount(vft::SdfTessellator::GLYPH_MESH_BOUNDING_BOX_BUFFER_INDEX), 0};
                geometry.uploadPrimitiveSetData(&primitiveSet, sizeof(CadR::PrimitiveSet));
            }

            // Create drawable for character
            CadR::Drawable &drawable = this->_drawables.emplace_back(*this->_renderer);
            drawable.create(
                this->_geometries.at(this->_geometryOffsets.at(key)), 0, sizeof(DrawableShaderData), 1,
                *this->_textureStateSets.at(this->_textureOffsets.at(character.getFont()->getFontFamily())));
            DrawableShaderData data{textBlock->getColor(), character.getModelMatrix()};
            drawable.uploadShaderData(&data, sizeof(DrawableShaderData));
        }
    }
}

void SdfTextRenderer::addFontAtlas(const vft::FontAtlas &atlas) {
    vft::SdfTextRenderer::addFontAtlas(atlas);

    this->_textureOffsets.insert({atlas.getFontFamily(), static_cast<unsigned int>(this->_textureOffsets.size())});

    // Copy texture data into staging buffer
    size_t bufferSize = atlas.getSize().x * atlas.getSize().y;
    CadR::StagingBuffer stagingBuffer{this->_renderer->imageStorage(), bufferSize, 1};
    memcpy(stagingBuffer.data(), atlas.getTexture().data(), bufferSize);

    // Create CadR image allocation
    CadR::ImageAllocation &imageAllocation = this->_images.emplace_back(this->_renderer->imageStorage());
    imageAllocation.alloc(vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::ImageCreateInfo{
                              vk::ImageCreateFlags{},                                                   // flags
                              vk::ImageType::e2D,                                                       // imageType
                              vk::Format::eR8Unorm,                                                     // format
                              vk::Extent3D{atlas.getSize().x, atlas.getSize().y, 1},                    // extent
                              1,                                                                        // mipLevels
                              1,                                                                        // arrayLayers
                              vk::SampleCountFlagBits::e1,                                              // samples
                              vk::ImageTiling::eOptimal,                                                // tiling
                              vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,  // usage
                              vk::SharingMode::eExclusive,                                              // sharingMode
                              0,                           // queueFamilyIndexCount
                              nullptr,                     // pQueueFamilyIndices
                              vk::ImageLayout::eUndefined  // initialLayout
                          },
                          this->_renderer->device());
    stagingBuffer.submit(imageAllocation,                                     // ImageAllocation
                         vk::ImageLayout::eUndefined,                         // currentLayout,
                         vk::ImageLayout::eTransferDstOptimal,                // copyLayout,
                         vk::ImageLayout::eShaderReadOnlyOptimal,             // newLayout,
                         vk::PipelineStageFlagBits::eFragmentShader,          // newLayoutBarrierDstStages,
                         vk::AccessFlagBits::eShaderRead,                     // newLayoutBarrierDstAccessFlags,
                         vk::Extent2D(atlas.getSize().x, atlas.getSize().y),  // imageExtent
                         bufferSize                                           // dataSize
    );

    // Create CadR texture
    CadR::Texture &texture =
        this->_textures.emplace_back(this->_images.at(this->_images.size() - 1),
                                     vk::ImageViewCreateInfo{vk::ImageViewCreateFlags{},  // flags
                                                             nullptr,                     // image
                                                             vk::ImageViewType::e2D,      // viewType
                                                             vk::Format::eR8Unorm,        // format
                                                             vk::ComponentMapping{
                                                                 vk::ComponentSwizzle::eR,
                                                                 vk::ComponentSwizzle::eG,
                                                                 vk::ComponentSwizzle::eB,
                                                                 vk::ComponentSwizzle::eA,
                                                             },  // components
                                                             vk::ImageSubresourceRange{
                                                                 vk::ImageAspectFlagBits::eColor,  // aspectMask
                                                                 0,                                // baseMipLevel
                                                                 1,                                // levelCount
                                                                 0,                                // baseArrayLayer
                                                                 1,                                // layerCount
                                                             }},                                   // subresourceRange
                                     this->_sampler.handle(), this->_renderer->device());

    // Create state set for texture
    std::unique_ptr<CadR::StateSet> &textureStateSet =
        this->_textureStateSets.emplace_back(std::make_unique<CadR::StateSet>(*this->_renderer));
    textureStateSet->allocDescriptorSet(vk::DescriptorType::eCombinedImageSampler,
                                        this->_pipelines[0].descriptorSetLayout(0));
    texture.attachStateSet(
        *textureStateSet, textureStateSet->descriptorSets()[0],
        [](CadR::StateSet &stateSet, vk::DescriptorSet descriptorSet, CadR::Texture &texture) -> void {
            stateSet.updateDescriptorSet(vk::WriteDescriptorSet{
                descriptorSet,                              // dstSet
                0,                                          // dstBinding
                0,                                          // dstArrayElement
                1,                                          // descriptorCount
                vk::DescriptorType::eCombinedImageSampler,  // descriptorType
                &(const vk::DescriptorImageInfo &)vk::DescriptorImageInfo{
                    texture.sampler(),                       // sampler
                    texture.imageView(),                     // imageView
                    vk::ImageLayout::eShaderReadOnlyOptimal  // imageLayout
                },                                           // pImageInfo
                nullptr,                                     // pBufferInfo
                nullptr                                      // pTexelBufferView
            });
        });

    // Add texture state set to parent state set
    this->_boundingBoxStateSet.childList.append(*textureStateSet);
}

void SdfTextRenderer::createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) {
    this->destroyPipelines();
    this->_pipelines.emplace_back(*this->_renderer);

    vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
        vk::PipelineShaderStageCreateFlags{},  // flags
        vk::ShaderStageFlagBits::eVertex,      // stage
        this->_vertexShaderModule,             // module
        "main",                                // pName
        nullptr                                // pSpecializationInfo
    };
    vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
        vk::PipelineShaderStageCreateFlags{},  // flags
        vk::ShaderStageFlagBits::eFragment,    // stage
        this->_fragmentShaderModule,           // module
        "main",                                // pName
        nullptr                                // pSpecializationInfo
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertexShaderStageCreateInfo,
                                                                     fragmentShaderStageCreateInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
        vk::PipelineVertexInputStateCreateFlags{},  // flags
        0,                                          // vertexBindingDescriptionCount
        nullptr,                                    // pVertexBindingDescriptions
        0,                                          // vertexAttributeDescriptionCount
        nullptr                                     // pVertexAttributeDescriptions
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        vk::PipelineInputAssemblyStateCreateFlags{},  // flags
        vk::PrimitiveTopology::eTriangleList,         // topology
        VK_FALSE                                      // primitiveRestartEnable
    };

    vk::Viewport viewport{
        0.f,                                // x
        0.f,                                // y
        static_cast<float>(extent.width),   // width
        static_cast<float>(extent.height),  // height
        0.f,                                // minDepth
        1.f                                 // maxDepth
    };
    vk::Rect2D scissor{
        vk::Offset2D{0, 0},  // offset
        extent               // extent
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
        vk::PipelineViewportStateCreateFlags{},  // flags
        1,                                       // viewportCount
        &viewport,                               // pViewports
        1,                                       // scissorCount
        &scissor                                 // pScissors
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
        vk::PipelineRasterizationStateCreateFlags{},  // flags
        VK_FALSE,                                     // depthClampEnable
        VK_FALSE,                                     // rasterizerDiscardEnable
        vk::PolygonMode::eFill,                       // polygonMode
        vk::CullModeFlagBits::eNone,                  // cullMode
        vk::FrontFace::eCounterClockwise,             // frontFace
        VK_FALSE,                                     // depthBiasEnable
        0.f,                                          // depthBiasConstantFactor
        0.f,                                          // depthBiasClamp
        0.f,                                          // depthBiasSlopeFactor
        1.0f                                          // lineWidth
    };

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
        vk::PipelineMultisampleStateCreateFlags{},  // flags
        vk::SampleCountFlagBits::e1,                // rasterizationSamples
        VK_FALSE,                                   // sampleShadingEnable
        0.f,                                        // minSampleShading
        nullptr,                                    // pSampleMask
        VK_FALSE,                                   // alphaToCoverageEnable
        VK_FALSE                                    // alphaToOneEnable
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{
        VK_TRUE,                             // blendEnable
        vk::BlendFactor::eSrcAlpha,          // srcColorBlendFactor
        vk::BlendFactor::eOneMinusSrcAlpha,  // dstColorBlendFactor
        vk::BlendOp::eAdd,                   // colorBlendOp
        vk::BlendFactor::eOne,               // srcAlphaBlendFactor
        vk::BlendFactor::eZero,              // dstAlphaBlendFactor
        vk::BlendOp::eAdd,                   // alphaBlendOp
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA  // colorWriteMask
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
        vk::PipelineColorBlendStateCreateFlags{},  // flags
        VK_FALSE,                                  // logicOpEnable
        vk::LogicOp::eClear,                       // logicOp
        1,                                         // attachmentCount
        &colorBlendAttachmentState                 // pAttachments
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        vk::PipelineDepthStencilStateCreateFlags{},  // flags
        VK_TRUE,                                     // depthTestEnable
        VK_TRUE,                                     // depthWriteEnable
        vk::CompareOp::eLess,                        // depthCompareOp
        VK_FALSE,                                    // depthBoundsTestEnable
        VK_FALSE,                                    // stencilTestEnable
        vk::StencilOpState(),                        // front
        vk::StencilOpState(),                        // back
        0.f,                                         // minDepthBounds
        0.f                                          // maxDepthBounds
    };

    vk::PushConstantRange pushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,  // stageFlags
        0,                                                                      // offset
        2 * sizeof(uint64_t)                                                    // size
    };

    // Descriptor set layout for font atlas
    vk::DescriptorSetLayout descriptorSetLayout =
        this->_renderer->device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
            vk::DescriptorSetLayoutCreateFlags{},  // flags
            1,                                     // bindingCount
            std::array<vk::DescriptorSetLayoutBinding, 1>{
                vk::DescriptorSetLayoutBinding{
                    0,                                          // binding
                    vk::DescriptorType::eCombinedImageSampler,  // descriptorType
                    1,                                          // descriptorCount
                    vk::ShaderStageFlagBits::eFragment,         // stageFlags
                    nullptr                                     // pImmutableSamplers
                }}
                .data()  // pBindings
        });
    this->_descriptorSetLayouts.push_back(descriptorSetLayout);

    // Create vulkan pipeline
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        vk::PipelineLayoutCreateFlags{},                            // flags
        static_cast<uint32_t>(this->_descriptorSetLayouts.size()),  // setLayoutCount
        this->_descriptorSetLayouts.data(),                         // pSetLayouts
        1,                                                          // pushConstantRangeCount
        &pushConstantRange                                          // pPushConstantRanges
    };
    vk::PipelineLayout pipelineLayout = this->_renderer->device().createPipelineLayout(pipelineLayoutCreateInfo);

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
        vk::PipelineCreateFlags{},                   // flags
        static_cast<uint32_t>(shaderStages.size()),  // stageCount
        shaderStages.data(),                         // pStages
        &vertexInputStateCreateInfo,                 // pVertexInputState
        &inputAssemblyStateCreateInfo,               // pInputAssemblyState
        nullptr,                                     // pTessellationState
        &viewportStateCreateInfo,                    // pViewportState
        &rasterizationStateCreateInfo,               // pRasterizationState
        &multisampleStateCreateInfo,                 // pMultisampleState
        &depthStencilCreateInfo,                     // pDepthStencilState
        &colorBlendStateCreateInfo,                  // pColorBlendState
        nullptr,                                     // pDynamicState
        pipelineLayout,                              // layout
        renderPass,                                  // renderPass
        0                                            // subpass
    };
    vk::Pipeline pipeline =
        this->_renderer->device().createGraphicsPipeline(this->_renderer->pipelineCache(), graphicsPipelineCreateInfo);

    // Init CadR pipeline
    this->_pipelines[0].init(pipeline, pipelineLayout, &this->_descriptorSetLayouts);

    // Init state set
    this->_boundingBoxStateSet.pipeline = &(this->_pipelines[0]);
    this->_boundingBoxStateSet.pipelineLayout = this->_pipelines[0].layout();
}

void SdfTextRenderer::createShaderModules() {
    vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},  // flags
        sizeof(vertexShaderSpirv),      // codeSize
        vertexShaderSpirv               // pCode
    };
    this->_vertexShaderModule = this->_renderer->device().createShaderModule(vertexShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},  // flags
        sizeof(fragmentShaderSpirv),    // codeSize
        fragmentShaderSpirv             // pCode
    };
    this->_fragmentShaderModule = this->_renderer->device().createShaderModule(fragmentShaderModuleCreateInfo);
}

}  // namespace CadText
