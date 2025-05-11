#include <CadText/TextRenderer.h>

namespace CadText {

TextRenderer::TextRenderer(CadR::Renderer &renderer) : _renderer{&renderer}, _rootStateSet{renderer} {}

TextRenderer::~TextRenderer() {
    this->destroyPipelines();
}

void TextRenderer::destroyPipelines() {
    for (CadR::Pipeline &pipeline : this->_pipelines) {
        pipeline.destroyDescriptorSetLayouts();
        pipeline.destroyPipelineLayout();
        pipeline.destroyPipeline();
    }

    this->_pipelines.clear();
}

void TextRenderer::setSceneData(vk::DeviceAddress sceneData) {
    this->_sceneData = sceneData;
}

CadR::StateSet &TextRenderer::stateSet() {
    return this->_rootStateSet;
}

std::vector<CadR::Pipeline> &TextRenderer::pipelines() {
    return this->_pipelines;
}

}  // namespace CadText
