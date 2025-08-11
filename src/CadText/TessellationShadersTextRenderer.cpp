#include <CadText/TessellationShadersTextRenderer.h>

namespace CadText {

const uint32_t TessellationShadersTextRenderer::innerVertexShaderSpirv[] = {
#include "shaders/triangle.vert.spv"
};

const uint32_t TessellationShadersTextRenderer::innerFragmentShaderSpirv[] = {
#include "shaders/triangle.frag.spv"
};

const uint32_t TessellationShadersTextRenderer::outerVertexShaderSpirv[] = {
#include "shaders/curve.vert.spv"
};

const uint32_t TessellationShadersTextRenderer::outerTessellationControlShaderSpirv[] = {
#include "shaders/curve.tesc.spv"
};

const uint32_t TessellationShadersTextRenderer::outerTessellationEvaluationShaderSpirv[] = {
#include "shaders/curve.tese.spv"
};

const uint32_t TessellationShadersTextRenderer::outerFragmentShaderSpirv[] = {
#include "shaders/curve.frag.spv"
};

TessellationShadersTextRenderer::TessellationShadersTextRenderer(CadR::Renderer &renderer,
                                                                 vk::RenderPass renderPass,
                                                                 vk::Extent2D extent)
    : CadText::TessellationShadersTextRenderer{renderer} {
    this->setViewportSize(extent.width, extent.height);
    this->createPipelines(renderPass, extent);
}

TessellationShadersTextRenderer::TessellationShadersTextRenderer(CadR::Renderer &renderer)
    : CadText::TextRenderer{renderer}, _innerStateSet{renderer}, _outerStateSet{renderer} {
    this->createShaderModules();

    this->_rootStateSet.childList.append(this->_innerStateSet);
    this->_rootStateSet.childList.append(this->_outerStateSet);

    this->_innerStateSet.recordCallList.push_back(
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
    this->_outerStateSet.recordCallList.push_back([&](CadR::StateSet &stateSet,
                                                      vk::CommandBuffer commandBuffer) -> void {
        this->_renderer->device().cmdPushConstants(
            commandBuffer,                 // commandBuffer
            this->_pipelines[1].layout(),  // pipelineLayout
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl |
                vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment,  // stageFlags
            0,                                                                                          // offset
            2 * sizeof(uint64_t),                                                                       // size
            std::array<uint64_t, 2>{
                this->_renderer->drawablePayloadDeviceAddress(),  // payloadBufferPtr
                this->_sceneData                                  // sceneDataPtr
            }
                .data()  // pValues
        );
    });
}

TessellationShadersTextRenderer::~TessellationShadersTextRenderer() {
    this->destroyPipelines();

    // Destroy shader modules
    this->_renderer->device().destroyShaderModule(this->_innerVertexShaderModule);
    this->_renderer->device().destroyShaderModule(this->_innerFragmentShaderModule);
    this->_renderer->device().destroyShaderModule(this->_outerVertexShaderModule);
    this->_renderer->device().destroyShaderModule(this->_outerTessellationControlShaderModule);
    this->_renderer->device().destroyShaderModule(this->_outerTessellationEvaluationShaderModule);
    this->_renderer->device().destroyShaderModule(this->_outerFragmentShaderModule);
}

void TessellationShadersTextRenderer::update() {
    this->_innerDrawables.clear();
    this->_innerGeometries.clear();
    this->_innerGeometryOffsets.clear();
    this->_outerDrawables.clear();
    this->_outerGeometries.clear();
    this->_outerGeometryOffsets.clear();

    for (std::shared_ptr<vft::TextBlock> textBlock : this->_textBlocks) {
        for (const vft::Character &character : textBlock->getCharacters()) {
            vft::GlyphKey key{character.getFont()->getFontFamily(), character.getGlyphId(), 0};

            // Insert glyph into cache
            if (!this->_innerGeometryOffsets.contains(key) && !this->_cache->exists(key)) {
                this->_cache->setGlyph(key, this->_tessellator->composeGlyph(
                                                character.getGlyphId(), character.getFont(), character.getFontSize()));
            }

            // Skip glyphs with no geometry
            const vft::Glyph &glyph = this->_cache->getGlyph(key);
            if (glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX) == 0 &&
                glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_CURVE_BUFFER_INDEX) == 0) {
                continue;
            }

            // Create inner geometry for glyph if neccessary
            if (!this->_innerGeometryOffsets.contains(key)) {
                CadR::Geometry &innerGeometry = this->_innerGeometries.emplace_back(*this->_renderer);
                this->_innerGeometryOffsets.insert(
                    {key, static_cast<unsigned int>(this->_innerGeometryOffsets.size())});

                innerGeometry.uploadVertexData(glyph.mesh.getVertices().data(),
                                               glyph.mesh.getVertexCount() * sizeof(glm::vec2));
                innerGeometry.uploadIndexData(
                    glyph.mesh.getIndices(vft::TessellationShadersTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX).data(),
                    glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX) *
                        sizeof(uint32_t));

                CadR::PrimitiveSet innerPrimitiveSet{
                    glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_TRIANGLE_BUFFER_INDEX), 0};
                innerGeometry.uploadPrimitiveSetData(&innerPrimitiveSet, sizeof(CadR::PrimitiveSet));
            }

            // Create outer geometry for glyph if neccessary
            if (!this->_outerGeometryOffsets.contains(key)) {
                CadR::Geometry &outerGeometry = this->_outerGeometries.emplace_back(*this->_renderer);
                this->_outerGeometryOffsets.insert(
                    {key, static_cast<unsigned int>(this->_outerGeometryOffsets.size())});

                outerGeometry.uploadVertexData(glyph.mesh.getVertices().data(),
                                               glyph.mesh.getVertexCount() * sizeof(glm::vec2));
                outerGeometry.uploadIndexData(
                    glyph.mesh.getIndices(vft::TessellationShadersTessellator::GLYPH_MESH_CURVE_BUFFER_INDEX).data(),
                    glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_CURVE_BUFFER_INDEX) *
                        sizeof(uint32_t));

                CadR::PrimitiveSet outerPrimitiveSet{
                    glyph.mesh.getIndexCount(vft::TessellationShadersTessellator::GLYPH_MESH_CURVE_BUFFER_INDEX), 0};
                outerGeometry.uploadPrimitiveSetData(&outerPrimitiveSet, sizeof(CadR::PrimitiveSet));
            }

            // Create drawable for inner triangles
            CadR::Drawable &innerDrawable = this->_innerDrawables.emplace_back(*this->_renderer);
            innerDrawable.create(this->_innerGeometries.at(this->_innerGeometryOffsets.at(key)), 0,
                                 sizeof(DrawableShaderData), 1, this->_innerStateSet);
            DrawableShaderData innerData{textBlock->getColor(), character.getModelMatrix()};
            innerDrawable.uploadShaderData(&innerData, sizeof(DrawableShaderData));

            // Create drawable for outer curves
            CadR::Drawable &outerDrawable = this->_outerDrawables.emplace_back(*this->_renderer);
            outerDrawable.create(this->_outerGeometries.at(this->_outerGeometryOffsets.at(key)), 0,
                                 sizeof(DrawableShaderData), 1, this->_outerStateSet);
            DrawableShaderData outerData{textBlock->getColor(), character.getModelMatrix()};
            outerDrawable.uploadShaderData(&outerData, sizeof(DrawableShaderData));
        }
    }
}

void TessellationShadersTextRenderer::createPipelines(vk::RenderPass renderPass, vk::Extent2D extent) {
    this->destroyPipelines();
    this->_pipelines.emplace_back(*this->_renderer);
    this->_pipelines.emplace_back(*this->_renderer);

    // Pipeline for inner triangles
    {
        vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},  // flags
            vk::ShaderStageFlagBits::eVertex,      // stage
            this->_innerVertexShaderModule,        // module
            "main",                                // pName
            nullptr                                // pSpecializationInfo
        };
        vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},  // flags
            vk::ShaderStageFlagBits::eFragment,    // stage
            this->_innerFragmentShaderModule,      // module
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
        vk::Pipeline pipeline = this->_renderer->device().createGraphicsPipeline(this->_renderer->pipelineCache(),
                                                                                 graphicsPipelineCreateInfo);

        // Init CadR pipeline
        this->_pipelines[0].init(pipeline, pipelineLayout, nullptr);

        // Init state set
        this->_innerStateSet.pipeline = &(this->_pipelines[0]);
        this->_innerStateSet.pipelineLayout = this->_pipelines[0].layout();
    }

    // Pipeline for outer quadratic bezier curves
    {
        vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},  // flags
            vk::ShaderStageFlagBits::eVertex,      // stage
            this->_outerVertexShaderModule,        // module
            "main",                                // pName
            nullptr                                // pSpecializationInfo
        };
        vk::PipelineShaderStageCreateInfo tessellationControlShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},           // flags
            vk::ShaderStageFlagBits::eTessellationControl,  // stage
            this->_outerTessellationControlShaderModule,    // module
            "main",                                         // pName
            nullptr                                         // pSpecializationInfo
        };
        vk::PipelineShaderStageCreateInfo tessellationEvaluationShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},              // flags
            vk::ShaderStageFlagBits::eTessellationEvaluation,  // stage
            this->_outerTessellationEvaluationShaderModule,    // module
            "main",                                            // pName
            nullptr                                            // pSpecializationInfo
        };
        vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
            vk::PipelineShaderStageCreateFlags{},  // flags
            vk::ShaderStageFlagBits::eFragment,    // stage
            this->_outerFragmentShaderModule,      // module
            "main",                                // pName
            nullptr                                // pSpecializationInfo
        };

        std::array<vk::PipelineShaderStageCreateInfo, 4> shaderStages = {
            vertexShaderStageCreateInfo, tessellationControlShaderStageCreateInfo,
            tessellationEvaluationShaderStageCreateInfo, fragmentShaderStageCreateInfo};

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
            vk::PipelineVertexInputStateCreateFlags{},  // flags
            0,                                          // vertexBindingDescriptionCount
            nullptr,                                    // pVertexBindingDescriptions
            0,                                          // vertexAttributeDescriptionCount
            nullptr                                     // pVertexAttributeDescriptions
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
            vk::PipelineInputAssemblyStateCreateFlags{},  // flags
            vk::PrimitiveTopology::ePatchList,            // topology
            VK_FALSE                                      // primitiveRestartEnable
        };

        vk::PipelineTessellationStateCreateInfo tessellationStateCreateInfo{
            vk::PipelineTessellationStateCreateFlags{},  // flags
            3                                            // patchControlPoints
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
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl |
                vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment,  // stageFlags
            0,                                                                                          // offset
            2 * sizeof(uint64_t)                                                                        // size
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
            &tessellationStateCreateInfo,                // pTessellationState
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
        vk::Pipeline pipeline = this->_renderer->device().createGraphicsPipeline(this->_renderer->pipelineCache(),
                                                                                 graphicsPipelineCreateInfo);

        // Init CadR pipeline
        this->_pipelines[1].init(pipeline, pipelineLayout, nullptr);

        // Init state set
        this->_outerStateSet.pipeline = &(this->_pipelines[1]);
        this->_outerStateSet.pipelineLayout = this->_pipelines[1].layout();
    }
}

void TessellationShadersTextRenderer::createShaderModules() {
    vk::ShaderModuleCreateInfo innerVertexShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},   // flags
        sizeof(innerVertexShaderSpirv),  // codeSize
        innerVertexShaderSpirv           // pCode
    };
    this->_innerVertexShaderModule = this->_renderer->device().createShaderModule(innerVertexShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo innerFragmentShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},     // flags
        sizeof(innerFragmentShaderSpirv),  // codeSize
        innerFragmentShaderSpirv           // pCode
    };
    this->_innerFragmentShaderModule =
        this->_renderer->device().createShaderModule(innerFragmentShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo outerVertexShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},   // flags
        sizeof(outerVertexShaderSpirv),  // codeSize
        outerVertexShaderSpirv           // pCode
    };
    this->_outerVertexShaderModule = this->_renderer->device().createShaderModule(outerVertexShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo outerTessellationControlShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},                // flags
        sizeof(outerTessellationControlShaderSpirv),  // codeSize
        outerTessellationControlShaderSpirv           // pCode
    };
    this->_outerTessellationControlShaderModule =
        this->_renderer->device().createShaderModule(outerTessellationControlShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo outerTessellationEvaluationShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},                   // flags
        sizeof(outerTessellationEvaluationShaderSpirv),  // codeSize
        outerTessellationEvaluationShaderSpirv           // pCode
    };
    this->_outerTessellationEvaluationShaderModule =
        this->_renderer->device().createShaderModule(outerTessellationEvaluationShaderModuleCreateInfo);

    vk::ShaderModuleCreateInfo outerFragmentShaderModuleCreateInfo{
        vk::ShaderModuleCreateFlags{},     // flags
        sizeof(outerFragmentShaderSpirv),  // codeSize
        outerFragmentShaderSpirv           // pCode
    };
    this->_outerFragmentShaderModule =
        this->_renderer->device().createShaderModule(outerFragmentShaderModuleCreateInfo);
}

}  // namespace CadText
