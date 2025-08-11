#include <VFONT/font.h>
#include <VFONT/font_atlas.h>
#include <VFONT/text_block.h>
#include <VFONT/text_block_builder.h>
#include <VFONT/unicode.h>

#include <CadText/SdfTextRenderer.h>
#include <CadText/TessellationShadersTextRenderer.h>
#include <CadText/TriangulationTextRenderer.h>
#include <CadText/WindingNumberTextRenderer.h>

#include <CadR/BoundingBox.h>
#include <CadR/BoundingSphere.h>
#include <CadR/Exceptions.h>
#include <CadR/ImageAllocation.h>
#include <CadR/StagingBuffer.h>
#include <CadR/StagingData.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "VulkanWindow.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // reduce amount of included files by windows.h
#include <windows.h>         // needed for SetConsoleOutputCP()
#endif

using namespace std;

struct SceneGpuData {
    unsigned int viewportWidth;
    unsigned int viewportHeight;
    glm::vec2 padding1;
    glm::mat4 view;        // current camera view matrix
    glm::mat4 projection;  // current camera projection matrix
};

// application class
class App {
public:
    const string FONT_PATH = "assets/Roboto-Regular.ttf";
    const u8string TM_TEXT = u8"Triangle meshes";
    const u8string TS_TEXT = u8"Tessellation shaders";
    const u8string WN_TEXT = u8"Winding number";
    const u8string SDF_TEXT = u8"Signed distance fields";

    App(int argc, char *argv[]);
    ~App();

    void init();
    void resize(VulkanWindow &window,
                const vk::SurfaceCapabilitiesKHR &surfaceCapabilities,
                vk::Extent2D newSurfaceExtent);
    void frame(VulkanWindow &window);
    void mouseMove(VulkanWindow &window, const VulkanWindow::MouseState &mouseState);
    void mouseButton(VulkanWindow &,
                     size_t button,
                     VulkanWindow::ButtonState buttonState,
                     const VulkanWindow::MouseState &mouseState);
    void mouseWheel(VulkanWindow &window, float wheelX, float wheelY, const VulkanWindow::MouseState &mouseState);

    // Vulkan core objects
    // (The order of members is not arbitrary but defines construction and destruction order.)
    // (If App object is on the stack, do not call std::exit() as the App destructor might not be called.)
    // (It is probably good idea to destroy vulkanLib and vulkanInstance after the display connection.)
    CadR::VulkanLibrary vulkanLib;
    CadR::VulkanInstance vulkanInstance;

    // window needs to be destroyed after the swapchain
    // (this is required especially on Wayland)
    VulkanWindow window;

    // Vulkan variables, handles and objects
    // (they need to be destructed in non-arbitrary order in the destructor)
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamily;
    uint32_t presentationQueueFamily;
    CadR::VulkanDevice device;
    vk::Queue graphicsQueue;
    vk::Queue presentationQueue;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Format depthFormat;
    float maxSamplerAnisotropy;
    vk::RenderPass renderPass;
    vk::SwapchainKHR swapchain;
    vector<vk::ImageView> swapchainImageViews;
    vector<vk::Framebuffer> framebuffers;
    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence renderFinishedFence;

    CadR::Renderer renderer;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    // camera control
    static constexpr const float maxZNearZFarRatio =
        1000.f;  // Maximum z-near distance and z-far distance ratio. High ratio might lead to low z-buffer precision.
    static constexpr const float zoomStepRatio =
        0.9f;  // Camera zoom speed. Camera distance from the model center is multiplied by this value on each mouse
               // wheel up event and divided by it on each wheel down event.
    float fovy =
        80.f / 180.f * glm::pi<float>();  // Initial field-of-view in y-axis (in vertical direction) is 80 degrees.
    float cameraHeading = 0.f;
    float cameraElevation = 0.f;
    float cameraDistance;
    float startMouseX, startMouseY;
    float startCameraHeading, startCameraElevation;
    CadR::BoundingBox sceneBoundingBox;
    CadR::BoundingSphere sceneBoundingSphere;

    std::shared_ptr<vft::Font> font;
    std::shared_ptr<vft::TextBlock> triangleMeshesTextBlock;
    std::shared_ptr<vft::TextBlock> tessellationShadersTextBlock;
    std::shared_ptr<vft::TextBlock> windingNumberTextBlock;
    std::shared_ptr<vft::TextBlock> sdfTextBlock;
    std::unique_ptr<CadText::TriangulationTextRenderer> triangleMeshesTextRenderer;
    std::unique_ptr<CadText::TessellationShadersTextRenderer> tessellationShadersTextRenderer;
    std::unique_ptr<CadText::WindingNumberTextRenderer> windingNumberTextRenderer;
    std::unique_ptr<CadText::SdfTextRenderer> sdfTextRenderer;

    CadR::HandlelessAllocation sceneDataAllocation;
    CadR::StateSet stateSetRoot;
};

class ExitWithMessage {
protected:
    int _exitCode;
    std::string _what;

public:
    ExitWithMessage(int exitCode, const std::string &msg) : _exitCode(exitCode), _what(msg) {}
    ExitWithMessage(int exitCode, const char *msg) : _exitCode(exitCode), _what(msg) {}
    const char *what() const noexcept { return _what.c_str(); }
    int exitCode() const noexcept { return _exitCode; }
};

/// Construct application object
App::App(int argc, char **argv)
    // none of the following initializators shall be allowed to throw,
    // otherwise a special care must be given to avoid memory leaks in the case of exception
    : sceneDataAllocation(
          renderer.dataStorage()),  // HandlelessAllocation(DataStorage&) does not throw, so it can be here
      stateSetRoot(renderer) {}

App::~App() {
    if (device) {
        // wait for device idle state
        // (to prevent errors during destruction of Vulkan resources)
        try {
            device.waitIdle();
        } catch (vk::Error &e) {
            cout << "Failed because of Vulkan exception: " << e.what() << endl;
        }

        // destroy text renderers
        triangleMeshesTextRenderer.reset();
        tessellationShadersTextRenderer.reset();
        windingNumberTextRenderer.reset();
        sdfTextRenderer.reset();

        // destroy handles
        // (the handles are destructed in certain (not arbitrary) order)
        sceneDataAllocation.free();
        device.destroy(commandPool);
        renderer.finalize();
        device.destroy(renderFinishedFence);
        device.destroy(renderFinishedSemaphore);
        device.destroy(imageAvailableSemaphore);
        for (auto f : framebuffers)
            device.destroy(f);
        for (auto v : swapchainImageViews)
            device.destroy(v);
        device.destroy(depthImage);
        device.freeMemory(depthImageMemory);
        device.destroy(depthImageView);
        device.destroy(swapchain);
        device.destroy(renderPass);
        device.destroy();
    }

    window.destroy();
#if defined(USE_PLATFORM_XLIB) || defined(USE_PLATFORM_QT)
    // On Xlib, VulkanWindow::finalize() needs to be called before instance destroy to avoid crash.
    // It is workaround for the known bug in libXext: https://gitlab.freedesktop.org/xorg/lib/libxext/-/issues/3,
    // that crashes the application inside XCloseDisplay(). The problem seems to be present
    // especially on Nvidia drivers (reproduced on versions 470.129.06 and 515.65.01, for example).
    //
    // On Qt, VulkanWindow::finalize() needs to be called not too late after leaving main() because
    // crash might follow. Probably Qt gets partly finalized. Seen on Linux with Qt 5.15.13 and Qt 6.4.2 on 2024-05-03.
    // Calling VulkanWindow::finalize() before leaving main() seems to be a safe solution. For simplicity, we are doing
    // it here.
    VulkanWindow::finalize();
#endif
    vulkanInstance.destroy();
}

void App::init() {
    // init Vulkan and window
    VulkanWindow::init();
    vulkanLib.load(CadR::VulkanLibrary::defaultName());
    vulkanInstance.create(vulkanLib, "Text", 0, "CADR", 0, VK_API_VERSION_1_2, nullptr,
                          VulkanWindow::requiredExtensions());
    window.create(vulkanInstance, {1024, 768}, "Text", vulkanLib.vkGetInstanceProcAddr);

    // init device and renderer
    tuple<vk::PhysicalDevice, uint32_t, uint32_t> deviceAndQueueFamilies = vulkanInstance.chooseDevice(
        vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute,          // queueOperations
        window.surface(),                                                    // presentationSurface
        [](CadR::VulkanInstance &instance, vk::PhysicalDevice pd) -> bool {  // filterCallback
            if (instance.getPhysicalDeviceProperties(pd).apiVersion < VK_API_VERSION_1_2)
                return false;
            auto features =
                instance.getPhysicalDeviceFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                                    vk::PhysicalDeviceVulkan12Features>(pd);
            return features.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect &&
                   features.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64 &&
                   features.get<vk::PhysicalDeviceFeatures2>().features.tessellationShader &&
                   features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                   features.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress;
            ;
        });
    physicalDevice = std::get<0>(deviceAndQueueFamilies);
    if (!physicalDevice)
        throw ExitWithMessage(2, "No compatible Vulkan device found.");
    device.create(vulkanInstance, deviceAndQueueFamilies,
#if 0  // enable or disable validation extensions
       // (0 enables validation extensions and features for debugging purposes)
                  "VK_KHR_swapchain",
                  []() {
                      CadR::Renderer::RequiredFeaturesStructChain f = CadR::Renderer::requiredFeaturesStructChain();
                      f.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
                      return f;
                  }()
                      .get<vk::PhysicalDeviceFeatures2>()
#else
                  {"VK_KHR_swapchain", "VK_KHR_shader_non_semantic_info"},
                  []() {
                      CadR::Renderer::RequiredFeaturesStructChain f = CadR::Renderer::requiredFeaturesStructChain();
                      f.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
                      f.get<vk::PhysicalDeviceFeatures2>().features.tessellationShader = true;
                      f.get<vk::PhysicalDeviceVulkan12Features>().uniformAndStorageBuffer8BitAccess = true;
                      return f;
                  }()
                      .get<vk::PhysicalDeviceFeatures2>()
#endif
    );
    graphicsQueueFamily = std::get<1>(deviceAndQueueFamilies);
    presentationQueueFamily = std::get<2>(deviceAndQueueFamilies);
    window.setDevice(device, physicalDevice);
    renderer.init(device, vulkanInstance, physicalDevice, graphicsQueueFamily);

    // get queues
    graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
    presentationQueue = device.getQueue(presentationQueueFamily, 0);

    // choose surface format
    surfaceFormat = [](vk::PhysicalDevice physicalDevice, CadR::VulkanInstance &vulkanInstance,
                       vk::SurfaceKHR surface) {
        constexpr const array candidateFormats{
            vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
            vk::SurfaceFormatKHR{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
            vk::SurfaceFormatKHR{vk::Format::eA8B8G8R8SrgbPack32, vk::ColorSpaceKHR::eSrgbNonlinear},
        };
        vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(surface, vulkanInstance);
        if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
            // Vulkan spec allowed single eUndefined value until 1.1.111 (2019-06-10)
            // with the meaning you can use any valid vk::Format value.
            // Now, it is forbidden, but let's handle any old driver.
            return candidateFormats[0];
        else {
            for (vk::SurfaceFormatKHR sf : availableFormats) {
                auto it = std::find(candidateFormats.begin(), candidateFormats.end(), sf);
                if (it != candidateFormats.end())
                    return *it;
            }
            if (availableFormats.size() ==
                0)  // Vulkan must return at least one format (this is mandated since Vulkan 1.0.37 (2016-10-10), but
                    // was missing in the spec before probably because of omission)
                throw std::runtime_error("Vulkan error: getSurfaceFormatsKHR() returned empty list.");
            return availableFormats[0];
        }
    }(physicalDevice, vulkanInstance, window.surface());

    // choose depth format
    depthFormat = [](vk::PhysicalDevice physicalDevice, CadR::VulkanInstance &vulkanInstance) {
        constexpr const array<vk::Format, 3> candidateFormats{
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint,
        };
        for (vk::Format f : candidateFormats) {
            vk::FormatProperties p = physicalDevice.getFormatProperties(f, vulkanInstance);
            if (p.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
                return f;
        }
        throw std::runtime_error("No suitable depth buffer format.");
    }(physicalDevice, vulkanInstance);

    // maxSamplerAnisotropy
    maxSamplerAnisotropy = vulkanInstance.getPhysicalDeviceProperties(physicalDevice).limits.maxSamplerAnisotropy;

    // render pass
    renderPass = device.createRenderPass(vk::RenderPassCreateInfo(
        vk::RenderPassCreateFlags(),  // flags
        2,                            // attachmentCount
        array<const vk::AttachmentDescription, 2>{
            // pAttachments
            vk::AttachmentDescription{
                // color attachment
                vk::AttachmentDescriptionFlags(),  // flags
                surfaceFormat.format,              // format
                vk::SampleCountFlagBits::e1,       // samples
                vk::AttachmentLoadOp::eClear,      // loadOp
                vk::AttachmentStoreOp::eStore,     // storeOp
                vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
                vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
                vk::ImageLayout::eUndefined,       // initialLayout
                vk::ImageLayout::ePresentSrcKHR    // finalLayout
            },
            vk::AttachmentDescription{
                // depth attachment
                vk::AttachmentDescriptionFlags(),                // flags
                depthFormat,                                     // format
                vk::SampleCountFlagBits::e1,                     // samples
                vk::AttachmentLoadOp::eClear,                    // loadOp
                vk::AttachmentStoreOp::eDontCare,                // storeOp
                vk::AttachmentLoadOp::eDontCare,                 // stencilLoadOp
                vk::AttachmentStoreOp::eDontCare,                // stencilStoreOp
                vk::ImageLayout::eUndefined,                     // initialLayout
                vk::ImageLayout::eDepthStencilAttachmentOptimal  // finalLayout
            },
        }
            .data(),
        1,                                                              // subpassCount
        &(const vk::SubpassDescription &)vk::SubpassDescription(        // pSubpasses
            vk::SubpassDescriptionFlags(),                              // flags
            vk::PipelineBindPoint::eGraphics,                           // pipelineBindPoint
            0,                                                          // inputAttachmentCount
            nullptr,                                                    // pInputAttachments
            1,                                                          // colorAttachmentCount
            &(const vk::AttachmentReference &)vk::AttachmentReference(  // pColorAttachments
                0,                                                      // attachment
                vk::ImageLayout::eColorAttachmentOptimal                // layout
                ),
            nullptr,                                                    // pResolveAttachments
            &(const vk::AttachmentReference &)vk::AttachmentReference(  // pDepthStencilAttachment
                1,                                                      // attachment
                vk::ImageLayout::eDepthStencilAttachmentOptimal         // layout
                ),
            0,       // preserveAttachmentCount
            nullptr  // pPreserveAttachments
            ),
        2,  // dependencyCount
        array{
            // pDependencies
            vk::SubpassDependency(
                VK_SUBPASS_EXTERNAL,     // srcSubpass
                0,                       // dstSubpass
                vk::PipelineStageFlags(  // srcStageMask
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eComputeShader |
                    vk::PipelineStageFlagBits::eTransfer),
                vk::PipelineStageFlags(  // dstStageMask
                    vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |
                    vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader |
                    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
                    vk::PipelineStageFlagBits::eColorAttachmentOutput),
                vk::AccessFlags(vk::AccessFlagBits::eShaderWrite |
                                vk::AccessFlagBits::eTransferWrite),  // srcAccessMask
                vk::AccessFlags(                                      // dstAccessMask
                    vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite),
                vk::DependencyFlags()  // dependencyFlags
                ),
            vk::SubpassDependency(0,                    // srcSubpass
                                  VK_SUBPASS_EXTERNAL,  // dstSubpass
                                  vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
                                  vk::PipelineStageFlags(vk::PipelineStageFlagBits::eBottomOfPipe),  // dstStageMask
                                  vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentWrite),
                                  vk::AccessFlags(),     // dstAccessMask
                                  vk::DependencyFlags()  // dependencyFlags
                                  ),
        }
            .data()

            ));

    // rendering semaphores and fences
    imageAvailableSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags()  // flags
                                                                             ));
    renderFinishedSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags()  // flags
                                                                             ));
    renderFinishedFence = device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled  // flags
                                                                 ));

    // command buffer
    commandPool = device.createCommandPool(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
        graphicsQueueFamily  // queueFamilyIndex
        ));
    commandBuffer =
        device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(commandPool,                       // commandPool
                                                                    vk::CommandBufferLevel::ePrimary,  // level
                                                                    1  // commandBufferCount
                                                                    ))[0];

    // Initialize text renderers
    triangleMeshesTextRenderer = std::make_unique<CadText::TriangulationTextRenderer>(
        renderer, renderPass, vk::Extent2D(window.surfaceExtent()));
    tessellationShadersTextRenderer = std::make_unique<CadText::TessellationShadersTextRenderer>(
        renderer, renderPass, vk::Extent2D(window.surfaceExtent()));
    windingNumberTextRenderer = std::make_unique<CadText::WindingNumberTextRenderer>(
        renderer, renderPass, vk::Extent2D(window.surfaceExtent()));
    sdfTextRenderer =
        std::make_unique<CadText::SdfTextRenderer>(renderer, renderPass, vk::Extent2D(window.surfaceExtent()));

    font = std::make_shared<vft::Font>(FONT_PATH);
    vft::FontAtlas fontAtlas{font, 64};
    sdfTextRenderer->addFontAtlas(fontAtlas);

    triangleMeshesTextBlock = vft::TextBlockBuilder()
                                  .setFont(font)
                                  .setFontSize(48)
                                  .setColor(glm::vec4(1.f, 1.f, 1.f, 1.f))
                                  .setPosition(glm::vec3(0.f, 0.f, 0.f))
                                  .build();
    triangleMeshesTextBlock->add(TM_TEXT);
    triangleMeshesTextRenderer->add(triangleMeshesTextBlock);

    tessellationShadersTextBlock = vft::TextBlockBuilder()
                                       .setFont(font)
                                       .setFontSize(48)
                                       .setColor(glm::vec4(1.f, 1.f, 1.f, 1.f))
                                       .setPosition(glm::vec3(0.f, 48.f, 0.f))
                                       .build();
    tessellationShadersTextBlock->add(TS_TEXT);
    tessellationShadersTextRenderer->add(tessellationShadersTextBlock);

    windingNumberTextBlock = vft::TextBlockBuilder()
                                 .setFont(font)
                                 .setFontSize(48)
                                 .setColor(glm::vec4(1.f, 1.f, 1.f, 1.f))
                                 .setPosition(glm::vec3(0.f, 96.f, 0.f))
                                 .build();
    windingNumberTextBlock->add(WN_TEXT);
    windingNumberTextRenderer->add(windingNumberTextBlock);

    sdfTextBlock = vft::TextBlockBuilder()
                       .setFont(font)
                       .setFontSize(48)
                       .setColor(glm::vec4(1.f, 1.f, 1.f, 1.f))
                       .setPosition(glm::vec3(0.f, 144.f, 0.f))
                       .build();
    sdfTextBlock->add(SDF_TEXT);
    sdfTextRenderer->add(sdfTextBlock);

    // Update root state set
    stateSetRoot.childList.append(triangleMeshesTextRenderer->stateSet());
    stateSetRoot.childList.append(tessellationShadersTextRenderer->stateSet());
    stateSetRoot.childList.append(windingNumberTextRenderer->stateSet());
    stateSetRoot.childList.append(sdfTextRenderer->stateSet());

    // scene bounding box
    sceneBoundingBox.min = glm::vec3{0.f, 0.f, 0.f};
    sceneBoundingBox.max =
        glm::vec3{sdfTextBlock->getWidth(), sdfTextBlock->getPosition().y + sdfTextBlock->getHeight(), 0.f};

    // scene bounding sphere
    sceneBoundingSphere.center = sceneBoundingBox.getCenter();
    sceneBoundingSphere.extendRadiusBy(glm::vec3{0.f, 0.f, 0.f});
    sceneBoundingSphere.radius *= 1.001f;  // increase radius to accommodate for all floating computations imprecisions

    // initial camera distance
    float fovy2Clamped = glm::clamp(fovy / 2.f, 1.f / 180.f * glm::pi<float>(), 90.f / 180.f * glm::pi<float>());
    cameraDistance = sceneBoundingSphere.radius / sin(fovy2Clamped);

    // upload all staging buffers
    renderer.executeCopyOperations();
}
void App::resize(VulkanWindow &window,
                 const vk::SurfaceCapabilitiesKHR &surfaceCapabilities,
                 vk::Extent2D newSurfaceExtent) {
    // clear resources
    for (auto v : swapchainImageViews)
        device.destroy(v);
    swapchainImageViews.clear();
    device.destroy(depthImage);
    device.free(depthImageMemory);
    device.destroy(depthImageView);
    for (auto f : framebuffers)
        device.destroy(f);
    framebuffers.clear();

    // update text renderers
    triangleMeshesTextRenderer->setViewportSize(newSurfaceExtent.width, newSurfaceExtent.height);
    tessellationShadersTextRenderer->setViewportSize(newSurfaceExtent.width, newSurfaceExtent.height);
    windingNumberTextRenderer->setViewportSize(newSurfaceExtent.width, newSurfaceExtent.height);
    sdfTextRenderer->setViewportSize(newSurfaceExtent.width, newSurfaceExtent.height);

    triangleMeshesTextRenderer->createPipelines(renderPass, newSurfaceExtent);
    tessellationShadersTextRenderer->createPipelines(renderPass, newSurfaceExtent);
    windingNumberTextRenderer->createPipelines(renderPass, newSurfaceExtent);
    sdfTextRenderer->createPipelines(renderPass, newSurfaceExtent);

    // print info
    cout << "Recreating swapchain (extent: " << newSurfaceExtent.width << "x" << newSurfaceExtent.height
         << ", extent by surfaceCapabilities: " << surfaceCapabilities.currentExtent.width << "x"
         << surfaceCapabilities.currentExtent.height << ", minImageCount: " << surfaceCapabilities.minImageCount
         << ", maxImageCount: " << surfaceCapabilities.maxImageCount << ")" << endl;

    // create new swapchain
    constexpr const uint32_t requestedImageCount = 2;
    vk::UniqueHandle<vk::SwapchainKHR, CadR::VulkanDevice> newSwapchain =
        device.createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR(
            vk::SwapchainCreateFlagsKHR(),          // flags
            window.surface(),                       // surface
            surfaceCapabilities.maxImageCount == 0  // minImageCount
                ? max(requestedImageCount, surfaceCapabilities.minImageCount)
                : clamp(requestedImageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
            surfaceFormat.format,                      // imageFormat
            surfaceFormat.colorSpace,                  // imageColorSpace
            newSurfaceExtent,                          // imageExtent
            1,                                         // imageArrayLayers
            vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
            (graphicsQueueFamily == presentationQueueFamily) ? vk::SharingMode::eExclusive
                                                             : vk::SharingMode::eConcurrent,  // imageSharingMode
            uint32_t(2),                                                                      // queueFamilyIndexCount
            array<uint32_t, 2>{graphicsQueueFamily, presentationQueueFamily}.data(),          // pQueueFamilyIndices
            surfaceCapabilities.currentTransform,                                             // preTransform
            vk::CompositeAlphaFlagBitsKHR::eOpaque,                                           // compositeAlpha
            vk::PresentModeKHR::eFifo,                                                        // presentMode
            VK_TRUE,                                                                          // clipped
            swapchain                                                                         // oldSwapchain
            ));
    device.destroy(swapchain);
    swapchain = newSwapchain.release();

    // swapchain images and image views
    vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain);
    swapchainImageViews.reserve(swapchainImages.size());
    for (vk::Image image : swapchainImages)
        swapchainImageViews.emplace_back(
            device.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),           // flags
                                                           image,                                // image
                                                           vk::ImageViewType::e2D,               // viewType
                                                           surfaceFormat.format,                 // format
                                                           vk::ComponentMapping(),               // components
                                                           vk::ImageSubresourceRange(            // subresourceRange
                                                               vk::ImageAspectFlagBits::eColor,  // aspectMask
                                                               0,                                // baseMipLevel
                                                               1,                                // levelCount
                                                               0,                                // baseArrayLayer
                                                               1                                 // layerCount
                                                               ))));

    // depth image
    depthImage = device.createImage(vk::ImageCreateInfo(
        vk::ImageCreateFlags(),                                                              // flags
        vk::ImageType::e2D,                                                                  // imageType
        depthFormat,                                                                         // format
        vk::Extent3D(newSurfaceExtent.width, newSurfaceExtent.height, 1),                    // extent
        1,                                                                                   // mipLevels
        1,                                                                                   // arrayLayers
        vk::SampleCountFlagBits::e1,                                                         // samples
        vk::ImageTiling::eOptimal,                                                           // tiling
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,  // usage
        vk::SharingMode::eExclusive,                                                         // sharingMode
        0,                                                                                   // queueFamilyIndexCount
        nullptr,                                                                             // pQueueFamilyIndices
        vk::ImageLayout::eUndefined                                                          // initialLayout
        ));

    // memory for images
    vk::PhysicalDeviceMemoryProperties memoryProperties =
        vulkanInstance.getPhysicalDeviceMemoryProperties(physicalDevice);
    auto allocateMemory = [](CadR::VulkanDevice &device, vk::Image image, vk::MemoryPropertyFlags requiredFlags,
                             const vk::PhysicalDeviceMemoryProperties &memoryProperties) -> vk::DeviceMemory {
        vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
            if (memoryRequirements.memoryTypeBits & (1 << i))
                if ((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags)
                    return device.allocateMemory(vk::MemoryAllocateInfo(memoryRequirements.size,  // allocationSize
                                                                        i                         // memoryTypeIndex
                                                                        ));
        throw std::runtime_error("No suitable memory type found for the image.");
    };
    depthImageMemory = allocateMemory(device, depthImage, vk::MemoryPropertyFlagBits::eDeviceLocal, memoryProperties);
    device.bindImageMemory(depthImage,        // image
                           depthImageMemory,  // memory
                           0                  // memoryOffset
    );

    // image views
    depthImageView = device.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(),  // flags
                                                                    depthImage,                  // image
                                                                    vk::ImageViewType::e2D,      // viewType
                                                                    depthFormat,                 // format
                                                                    vk::ComponentMapping(),      // components
                                                                    vk::ImageSubresourceRange(   // subresourceRange
                                                                        vk::ImageAspectFlagBits::eDepth,  // aspectMask
                                                                        0,  // baseMipLevel
                                                                        1,  // levelCount
                                                                        0,  // baseArrayLayer
                                                                        1   // layerCount
                                                                        )));

    // framebuffers
    framebuffers.reserve(swapchainImages.size());
    for (size_t i = 0, c = swapchainImages.size(); i < c; i++)
        framebuffers.emplace_back(
            device.createFramebuffer(vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(),  // flags
                                                               renderPass,                    // renderPass
                                                               2,                             // attachmentCount
                                                               array{
                                                                   // pAttachments
                                                                   swapchainImageViews[i],
                                                                   depthImageView,
                                                               }
                                                                   .data(),
                                                               newSurfaceExtent.width,   // width
                                                               newSurfaceExtent.height,  // height
                                                               1                         // layers
                                                               )));
}

void App::frame(VulkanWindow &) {
    // wait for previous frame rendering work
    // if still not finished
    // (we might start copy operations before, but we need to exclude TableHandles that must stay intact
    // until the rendering is finished)
    vk::Result r = device.waitForFences(renderFinishedFence,  // fences
                                        VK_TRUE,              // waitAll
                                        uint64_t(3e9)         // timeout
    );
    if (r != vk::Result::eSuccess) {
        if (r == vk::Result::eTimeout)
            throw runtime_error("GPU timeout. Task is probably hanging on GPU.");
        throw runtime_error("Vulkan error: vkWaitForFences failed with error " + to_string(r) + ".");
    }
    device.resetFences(renderFinishedFence);

    // _sceneDataAllocation
    uint32_t sceneDataSize = uint32_t(sizeof(SceneGpuData));
    CadR::StagingData sceneStagingData = sceneDataAllocation.alloc(sceneDataSize);
    SceneGpuData *sceneData = sceneStagingData.data<SceneGpuData>();
    sceneData->viewportWidth = window.surfaceExtent().width;
    sceneData->viewportHeight = window.surfaceExtent().height;
    sceneData->view = glm::lookAtLH(             // 0,0,+5 is produced inside translation part of viewMatrix
        sceneBoundingSphere.center + glm::vec3(  // eye
                                         +cameraDistance * sin(-cameraHeading) * cos(cameraElevation),  // x
                                         -cameraDistance * sin(cameraElevation),                        // y
                                         -cameraDistance * cos(cameraHeading) * cos(cameraElevation)    // z
                                         ),
        sceneBoundingSphere.center,  // center
        glm::vec3(0.f, 1.f, 0.f)     // up
    );
    float zFar = fabs(cameraDistance) + sceneBoundingSphere.radius;
    float zNear = fabs(cameraDistance) - sceneBoundingSphere.radius;
    float minZNear = zFar / maxZNearZFarRatio;
    if (zNear < minZNear)
        zNear = minZNear;
    sceneData->projection =
        glm::perspectiveLH_ZO(fovy, float(window.surfaceExtent().width) / window.surfaceExtent().height, zNear, zFar);

    // Set scene data ptr in text renderer
    triangleMeshesTextRenderer->setSceneData(sceneDataAllocation.deviceAddress());
    tessellationShadersTextRenderer->setSceneData(sceneDataAllocation.deviceAddress());
    windingNumberTextRenderer->setSceneData(sceneDataAllocation.deviceAddress());
    sdfTextRenderer->setSceneData(sceneDataAllocation.deviceAddress());

    // begin the frame
    renderer.beginFrame();

    // submit all copy operations that were not submitted yet
    renderer.executeCopyOperations();

    // acquire image
    uint32_t imageIndex;
    r = device.acquireNextImageKHR(swapchain,                // swapchain
                                   uint64_t(3e9),            // timeout (3s)
                                   imageAvailableSemaphore,  // semaphore to signal
                                   vk::Fence(nullptr),       // fence to signal
                                   &imageIndex               // pImageIndex
    );
    if (r != vk::Result::eSuccess) {
        if (r == vk::Result::eSuboptimalKHR) {
            window.scheduleResize();
            return;
        } else if (r == vk::Result::eErrorOutOfDateKHR) {
            window.scheduleResize();
            return;
        } else
            throw runtime_error("Vulkan error: vkAcquireNextImageKHR failed with error " + to_string(r) + ".");
    }

    // begin command buffer recording
    renderer.beginRecording(commandBuffer);

    // prepare recording
    size_t numDrawables = renderer.prepareSceneRendering(stateSetRoot);

    // record compute shader preprocessing
    renderer.recordDrawableProcessing(commandBuffer, numDrawables);

    // record scene rendering
    renderer.recordSceneRendering(commandBuffer,                                           // commandBuffer
                                  stateSetRoot,                                            // stateSetRoot
                                  renderPass,                                              // renderPass
                                  framebuffers[imageIndex],                                // framebuffer
                                  vk::Rect2D(vk::Offset2D(0, 0), window.surfaceExtent()),  // renderArea
                                  2,                                                       // clearValueCount
                                  array<vk::ClearValue, 2>{
                                      // pClearValues
                                      vk::ClearColorValue(array<float, 4>{0.f, 0.f, 0.f, 1.f}),
                                      vk::ClearDepthStencilValue(1.f, 0),
                                  }
                                      .data());

    // end command buffer recording
    renderer.endRecording(commandBuffer);

    // submit all copy operations that were not submitted yet
    renderer.executeCopyOperations();

    // submit frame
    device.queueSubmit(graphicsQueue,                               // queue
                       vk::SubmitInfo(1, &imageAvailableSemaphore,  // waitSemaphoreCount + pWaitSemaphores +
                                      &(const vk::PipelineStageFlags &)vk::PipelineStageFlags(  // pWaitDstStageMask
                                          vk::PipelineStageFlagBits::eColorAttachmentOutput),
                                      1, &commandBuffer,           // commandBufferCount + pCommandBuffers
                                      1, &renderFinishedSemaphore  // signalSemaphoreCount + pSignalSemaphores
                                      ),
                       renderFinishedFence  // fence
    );

    // present
    r = device.presentKHR(presentationQueue,                                // queue
                          &(const vk::PresentInfoKHR &)vk::PresentInfoKHR(  // presentInfo
                              1, &renderFinishedSemaphore,                  // waitSemaphoreCount + pWaitSemaphores
                              1, &swapchain, &imageIndex,  // swapchainCount + pSwapchains + pImageIndices
                              nullptr                      // pResults
                              ));
    if (r != vk::Result::eSuccess) {
        if (r == vk::Result::eSuboptimalKHR) {
            window.scheduleResize();
            cout << "present result: Suboptimal" << endl;
        } else if (r == vk::Result::eErrorOutOfDateKHR) {
            window.scheduleResize();
            cout << "present error: OutOfDate" << endl;
        } else
            throw runtime_error("Vulkan error: vkQueuePresentKHR() failed with error " + to_string(r) + ".");
    }
}

void App::mouseMove(VulkanWindow &window, const VulkanWindow::MouseState &mouseState) {
    if (mouseState.buttons[VulkanWindow::MouseButton::Left]) {
        cameraHeading = startCameraHeading + (mouseState.posX - startMouseX) * 0.01f;
        cameraElevation = startCameraElevation + (mouseState.posY - startMouseY) * 0.01f;
        window.scheduleFrame();
    }
}

void App::mouseButton(VulkanWindow &window,
                      size_t button,
                      VulkanWindow::ButtonState buttonState,
                      const VulkanWindow::MouseState &mouseState) {
    if (button == VulkanWindow::MouseButton::Left) {
        if (buttonState == VulkanWindow::ButtonState::Pressed) {
            startMouseX = mouseState.posX;
            startMouseY = mouseState.posY;
            startCameraHeading = cameraHeading;
            startCameraElevation = cameraElevation;
        } else {
            cameraHeading = startCameraHeading + (mouseState.posX - startMouseX) * 0.01f;
            cameraElevation = startCameraElevation + (mouseState.posY - startMouseY) * 0.01f;
            window.scheduleFrame();
        }
    }
}

void App::mouseWheel(VulkanWindow &window, float wheelX, float wheelY, const VulkanWindow::MouseState &mouseState) {
    cameraDistance *= pow(zoomStepRatio, wheelY);
    window.scheduleFrame();
}

int main(int argc, char *argv[]) {
    try {
        // set console code page to utf-8 to print non-ASCII characters correctly
#ifdef _WIN32
        if (!SetConsoleOutputCP(CP_UTF8))
            cout << "Failed to set console code page to utf-8." << endl;
#endif

        App app(argc, argv);
        app.init();
        app.window.setResizeCallback(bind(&App::resize, &app, placeholders::_1, placeholders::_2, placeholders::_3));
        app.window.setFrameCallback(bind(&App::frame, &app, placeholders::_1));
        app.window.setMouseMoveCallback(bind(&App::mouseMove, &app, placeholders::_1, placeholders::_2));
        app.window.setMouseButtonCallback(
            bind(&App::mouseButton, &app, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4));
        app.window.setMouseWheelCallback(
            bind(&App::mouseWheel, &app, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4));

        // show window and run main loop
        app.window.show();
        VulkanWindow::mainLoop();

        // finish all pending work on device
        app.device.waitIdle();

        // catch exceptions
    } catch (CadR::Error &e) {
        cout << "Failed because of CadR exception: " << e.what() << endl;
        return 9;
    } catch (vk::Error &e) {
        cout << "Failed because of Vulkan exception: " << e.what() << endl;
        return 9;
    } catch (ExitWithMessage &e) {
        cout << e.what() << endl;
        return e.exitCode();
    } catch (exception &e) {
        cout << "Failed because of exception: " << e.what() << endl;
        return 9;
    } catch (...) {
        cout << "Failed because of unspecified exception." << endl;
        return 9;
    }

    return 0;
}