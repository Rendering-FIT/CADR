#include <CadText/TriangulationTextRenderer.h>

namespace CadText {

const uint32_t TriangulationTextRenderer::vertexShaderSpirv[] = {
#include "shaders/triangle.vert.spv"
};

const uint32_t TriangulationTextRenderer::fragmentShaderSpirv[] = {
#include "shaders/triangle.frag.spv"
};

TriangulationTextRenderer::TriangulationTextRenderer(CadR::Renderer &renderer,
                                                     vk::RenderPass renderPass,
                                                     vk::Extent2D extent)
    : CadText::TriangulationTextRenderer{renderer} {
    this->setViewportSize(extent.width, extent.height);
    this->createPipelines(renderPass, extent);
}

TriangulationTextRenderer::TriangulationTextRenderer(CadR::Renderer &renderer)
    : CadText::TextRenderer{renderer}, _triangleStateSet{renderer} {
    this->createShaderModules();

    this->_rootStateSet.childList.append(this->_triangleStateSet);

    this->_triangleStateSet.recordCallList.push_back(
        [&](CadR::StateSet &stateSet, vk::CommandBuffer commandBuffer) -> void {
            this->_renderer->device().cmdPushConstants(
                commandBuffer,                     // commandBuffer
                this->_pipelines[0].layout(),      // pipelineLayout
                vk::ShaderStageFlagBits::eVertex,  // stageFlags
                0,                                 // offset
                2 * sizeof(uint64_t),              // size
                std::array<uint64_t, 2>{
                    this->_renderer->drawablePayloadDeviceAddress(),  // payloadBufferPtr
                    this->_sceneData                                  // sceneDataPtr
                }
                    .data()  // pValues
            );
        });
}

TriangulationTextRenderer::~TriangulationTextRenderer() {
    this->destroyPipelines();

    // Destroy shader modules
    this->_renderer->device().destroyShaderModule(this->_vertexShaderModule);
    this->_renderer->device().destroyShaderModule(this->_fragmentShaderModule);
}

void TriangulationTextRenderer::update() {
    this->_drawables.clear();
    this->_geometries.clear();
    this->_geometryOffsets.clear();

    for (std::shared_ptr<vft::TextBlock> textBlock : this->_textBlocks) {
        for (const vft::Character &character : textBlock->getCharacters()) {
            vft::GlyphKey key{character.getFont()->getFontFamily(), character.getGlyphId(), character.getFontSize()};

            // Insert glyph into cache
            if (!this->_geometryOffsets.contains(key) && !this->_cache->exists(key)) {
                this->_cache->setGlyph(key, this->_tessellator->composeGlyph(
                                                character.getGlyphId(), character.getFont(), character.getFontSize()));
            }

            // Skip glyphs with no geometry
            const vft::Glyph &glyph = this->_cache->getGlyph(key);
            if (glyph.mesh.getIndexCount(vft::TriangulationTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX) == 0) {
                continue;
            }

            // Create geometry for glyph if neccessary
            if (!this->_geometryOffsets.contains(key)) {
                CadR::Geometry &geometry = this->_geometries.emplace_back(*this->_renderer);
                this->_geometryOffsets.insert({key, static_cast<unsigned int>(this->_geometryOffsets.size())});

                geometry.uploadVertexData(glyph.mesh.getVertices().data(),
                                          glyph.mesh.getVertexCount() * sizeof(glm::vec2));
                geometry.uploadIndexData(
                    glyph.mesh.getIndices(vft::TriangulationTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX).data(),
                    glyph.mesh.getIndexCount(vft::TriangulationTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX) *
                        sizeof(uint32_t));

                CadR::PrimitiveSet primitiveSet{
                    glyph.mesh.getIndexCount(vft::TriangulationTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX), 0};
                geometry.uploadPrimitiveSetData(&primitiveSet, sizeof(CadR::PrimitiveSet));
            }

            // Create drawable for character
            CadR::Drawable &drawable = this->_drawables.emplace_back(*this->_renderer);
            drawable.create(this->_geometries.at(this->_geometryOffsets.at(key)), 0, sizeof(DrawableShaderData), 1,
                            this->_triangleStateSet);
            DrawableShaderData data{textBlock->getColor(), character.getModelMatrix()};
            drawable.uploadShaderData(&data, sizeof(DrawableShaderData));
        }
    }
}

void TriangulationTextRenderer::createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) {
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
        vk::ShaderStageFlagBits::eVertex,  // stageFlags
        0,                                 // offset
        2 * sizeof(uint64_t)               // size
    };

    // Create vulkan pipeline
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        vk::PipelineLayoutCreateFlags{},  // flags
        0,                                // setLayoutCount
        nullptr,                          // pSetLayouts
        1,                                // pushConstantRangeCount
        &pushConstantRange                // pPushConstantRanges
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
    this->_pipelines[0].init(pipeline, pipelineLayout, nullptr);

    // Init state set
    this->_triangleStateSet.pipeline = &(this->_pipelines[0]);
    this->_triangleStateSet.pipelineLayout = this->_pipelines[0].layout();
}

void TriangulationTextRenderer::createShaderModules() {
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
