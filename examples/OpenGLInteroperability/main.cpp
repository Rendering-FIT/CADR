#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
# define WIN32_LEAN_AND_MEAN  // this reduces win32 headers default namespace pollution
#else
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <GL/glx.h>
# include <unistd.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>
#include <GL/gl.h>
#include <GL/glext.h>
#include <array>
#include <iostream>
#include <set>

using namespace std;


// Vulkan instance
// (must be destructed as the last one, at least on Linux, it must be destroyed after display connection)
static vk::UniqueInstance instance;

// windowing variables
#ifdef _WIN32
static HWND window=nullptr;
static HWND glWindow=nullptr;
static HDC hDC=nullptr;
static HGLRC hRC=nullptr;
struct Win32Cleaner {
	~Win32Cleaner() {
		if(glWindow) {
			if(hRC) {
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(hRC);
			}
			if(hDC)
				ReleaseDC(glWindow,hDC);
			DestroyWindow(glWindow);
		}
		if(window) {
			DestroyWindow(window);
			UnregisterClass("HelloWindow",GetModuleHandle(NULL));
		}
	}
} win32Cleaner;
#else
static Display* display=nullptr;
static Window window=0;
static Window glWindow=0;
static Colormap glWindowColormap=0;
static GLXContext cx=nullptr;
struct XlibCleaner {
	~XlibCleaner() {
		if(cx) {
			glXMakeCurrent(display,None,NULL);
			glXDestroyContext(display,cx);
		}
		if(glWindow)  XDestroyWindow(display,glWindow);
		if(glWindowColormap!=0)
			XFreeColormap(display,glWindowColormap);
		if(window)  XDestroyWindow(display,window);
		if(display)  XCloseDisplay(display);
	}
} xlibCleaner;
Atom wmDeleteMessage;
#endif
static uint32_t windowWidth;
static uint32_t windowHeight;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order as it gives their destruction order)
static vk::UniqueSurfaceKHR surface;
static vk::PhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamily;
static uint32_t presentationQueueFamily;
static vk::UniqueDevice device;
static vk::Queue graphicsQueue;
static vk::Queue presentationQueue;
static vk::SurfaceFormatKHR chosenSurfaceFormat;
static vk::Format depthFormatVk;
static GLenum depthFormatGL;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule vsTwoTrianglesModule;
static vk::UniqueShaderModule fsTwoTrianglesModule;
static vk::UniqueShaderModule vsMergeModule;
static vk::UniqueShaderModule fsMergeModule;
static vk::UniquePipelineCache pipelineCache;
static vk::UniqueDescriptorSetLayout mergeDescriptorSetLayout;
static vk::UniquePipelineLayout twoTrianglesPipelineLayout;
static vk::UniquePipelineLayout mergePipelineLayout;
static vk::UniqueSwapchainKHR swapchain;
static vector<vk::UniqueImageView> swapchainImageViews;
static vk::UniqueImage depthImage;
static vk::UniqueDeviceMemory depthImageMemory;
static vk::UniqueImageView depthImageView;
static vk::UniqueSampler sampler;
static vk::UniquePipeline twoTrianglesPipeline;
static vk::UniquePipeline mergePipeline;
static vk::UniqueDescriptorPool mergeDescriptorPool;
static vk::UniqueDescriptorSet mergeDescriptorSet;
static vector<vk::UniqueFramebuffer> framebuffers;
static vk::UniqueCommandPool commandPool;
static vector<vk::UniqueCommandBuffer> commandBuffers;
static vk::UniqueSemaphore acquireCompleteSemaphore;
static vk::UniqueSemaphore renderFinishedSemaphore;
static vk::UniqueCommandBuffer transitionCommandBuffer;

// shared resources (Vulkan)
static vk::UniqueImage sharedColorImageVk;
static vk::UniqueImage sharedDepthImageVk;
static vk::UniqueDeviceMemory sharedColorImageMemoryVk;
static vk::UniqueDeviceMemory sharedDepthImageMemoryVk;
static vk::UniqueImageView sharedColorImageView;
static vk::UniqueImageView sharedDepthImageView;
static vk::UniqueSemaphore glStartSemaphoreVk;
static vk::UniqueSemaphore glDoneSemaphoreVk;

// Vulkan function pointers
struct VkFuncs {
	PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
#if 0
	PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR;
#endif
#ifdef _WIN32
	PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
#else
	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
	PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
#endif
};
static VkFuncs vkFuncs;

// OpenGL function pointers
static PFNGLCREATETEXTURESPROC glCreateTextures;
static PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT;
static PFNGLTEXTURESTORAGEMEM2DEXTPROC glTextureStorageMem2DEXT;
static PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXT;
static PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXT;
static PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXT;
static PFNGLGETUNSIGNEDBYTEI_VEXTPROC glGetUnsignedBytei_vEXT;
#ifdef _WIN32
static PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC glImportMemoryWin32HandleEXT;
static PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC glImportSemaphoreWin32HandleEXT;
#else
static PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT;
static PFNGLIMPORTSEMAPHOREFDEXTPROC glImportSemaphoreFdEXT;
#endif
static PFNGLCREATEFRAMEBUFFERSPROC glCreateFramebuffers;
static PFNGLNAMEDFRAMEBUFFERTEXTUREPROC glNamedFramebufferTexture;
static PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC glCheckNamedFramebufferStatus;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
static PFNGLWAITSEMAPHOREEXTPROC glWaitSemaphoreEXT;
static PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXT;
static PFNGLCLIPCONTROLPROC glClipControl;

// OpenGL texture and memory
struct UniqueGlTexture {
	GLuint texture = 0;
	~UniqueGlTexture()  { if(texture!=0) glDeleteTextures(1,&texture); }  // glDeleteTextures() since OpenGL 1.1
};
struct UniqueGlMemory {
	GLuint memory = 0;
	~UniqueGlMemory()  { if(memory!=0) glDeleteMemoryObjectsEXT(1,&memory); }
};
struct UniqueGlSemaphore {
	GLuint semaphore = 0;
	~UniqueGlSemaphore() { if(semaphore!=0) glDeleteSemaphoresEXT(1,&semaphore); }
};
struct UniqueGlFramebuffer {
	GLuint framebuffer = 0;
	~UniqueGlFramebuffer() { if(framebuffer!=0) glDeleteFramebuffers(1,&framebuffer); }
};

// shared resources (OpenGL)
static UniqueGlTexture sharedColorTextureGL;
static UniqueGlTexture sharedDepthTextureGL;
static UniqueGlMemory sharedColorTextureMemoryGL;
static UniqueGlMemory sharedDepthTextureMemoryGL;
static UniqueGlSemaphore glStartSemaphoreGL;
static UniqueGlSemaphore glDoneSemaphoreGL;
static UniqueGlFramebuffer glFramebuffer;


// shader code in SPIR-V binary
static const uint32_t vsTwoTrianglesSpirv[]={
#include "twoTriangles.vert.spv"
};
static const uint32_t fsTwoTrianglesSpirv[]={
#include "twoTriangles.frag.spv"
};
static const uint32_t vsMergeSpirv[]={
#include "merge.vert.spv"
};
static const uint32_t fsMergeSpirv[]={
#include "merge.frag.spv"
};


struct UUID {

	uint8_t data[VK_UUID_SIZE];

	inline UUID() = default;
	inline UUID(const uint8_t (&rhs)[VK_UUID_SIZE])  { memcpy(data,rhs,sizeof(data)); }  // required by vulkan.hpp prior to Vulkan SDK version 1.2.137 (release date 2020-04-07)
	inline UUID(const array<uint8_t,VK_UUID_SIZE>& rhs)  { memcpy(data,rhs.data(),sizeof(data)); }  // required by vulkan.hpp since Vulkan SDK 1.2.137 (release date 2020-04-07)

	inline void fillByZeros()  { memset(data,0,sizeof(data)); }

	inline bool operator==(const UUID& rhs) const  { return memcmp(data,rhs.data,sizeof(data))==0; }
	inline bool operator!=(const UUID& rhs) const  { return memcmp(data,rhs.data,sizeof(data))!=0; }

	std::string to_string() const {
		std::string s(VK_UUID_SIZE*2+4,'\0');
		unsigned minusPos=0x09070503; // each byte holds one position of '-'
		for(unsigned i=0,j=0; i<VK_UUID_SIZE; i++) {
			uint8_t b=data[i];
			uint8_t c=b>>4;
			s[j]=(c<=9)?'0'+c:'a'+c-10;
			c=b&0xf;
			j++;
			s[j]=(c<=9)?'0'+c:'a'+c-10;
			j++;
			if(i==(minusPos&0xff)) {
				s[j]='-';
				j++;
				minusPos>>=8;
			}
		}
		return s;
	}
};


template<typename T>
T glGetProcAddress(const std::string& funcName)
{
#ifdef _WIN32
	return reinterpret_cast<T>(wglGetProcAddress(funcName.c_str()));
#else
	return reinterpret_cast<T>(glXGetProcAddressARB((const GLubyte*)funcName.c_str()));
#endif
}


/// Init Vulkan and open the window.
static void init()
{
	// Vulkan instance
	instance=
		vk::createInstanceUnique(
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"CADR OpenGLInteroperability", // application name
					VK_MAKE_VERSION(0,0,0),  // application version
					"CADR",                  // engine name
					VK_MAKE_VERSION(0,0,0),  // engine version
					VK_API_VERSION_1_0,      // api version
				},
				0,nullptr,  // no layers
				5,          // enabled extension count
				array<const char*,5>{
					"VK_KHR_surface",
#ifdef _WIN32
					"VK_KHR_win32_surface",
#else
					"VK_KHR_xlib_surface",
#endif
					"VK_KHR_external_memory_capabilities",    // dependency of VK_KHR_external_memory (device extension)
					"VK_KHR_external_semaphore_capabilities", // dependency for VK_KHR_external_semaphore (device extension)
					"VK_KHR_get_physical_device_properties2"  // dependency of VK_KHR_external_memory_capabilities and VK_KHR_external_semaphore_capabilities
				}.data(),  // enabled extension names
			});

	// get instance function pointers
	vkFuncs.vkGetPhysicalDeviceProperties2KHR=PFN_vkGetPhysicalDeviceProperties2KHR(instance->getProcAddr("vkGetPhysicalDeviceProperties2KHR"));


#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw std::runtime_error("GetWindowRect() failed.");
	windowWidth=(screenSize.right-screenSize.left)/2;
	windowHeight=(screenSize.bottom-screenSize.top)/2;

	// window's message handling procedure
	auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
		switch(msg)
		{
			case WM_SIZE:
				windowWidth=LOWORD(lParam);
				windowHeight=HIWORD(lParam);
				return DefWindowProc(hwnd,msg,wParam,lParam);
			case WM_CLOSE:
				DestroyWindow(hwnd);
				UnregisterClass("HelloWindow",GetModuleHandle(NULL));
				window=nullptr;
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return DefWindowProc(hwnd,msg,wParam,lParam);
		}
	};

	// register window class
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "HelloWindow";
	wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
	if(!RegisterClassEx(&wc))
		throw std::runtime_error("Can not register window class.");

	// create Vulkan window
	window=CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"HelloWindow",
		"Hello window!",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,
		NULL,NULL,wc.hInstance,NULL);
	if(window==NULL) {
		UnregisterClass("HelloWindow",GetModuleHandle(NULL));
		throw std::runtime_error("Can not create window.");
	}

	// create OpenGL window
	glWindow=CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"HelloWindow",
		"Hello window!",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,windowWidth,windowHeight,
		NULL,NULL,wc.hInstance,NULL);
	if(glWindow==NULL) {
		UnregisterClass("HelloWindow",GetModuleHandle(NULL));
		throw std::runtime_error("Can not create window.");
	}

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),  // size of the structure
		1,                              // version
		PFD_DRAW_TO_WINDOW |            // format must support window
			PFD_DOUBLEBUFFER |
			PFD_SUPPORT_OPENGL,          // format must support OpenGL
		PFD_TYPE_RGBA,                  // request an RGBA format
		BYTE(32),                        // color depth
		0, 0, 0, 0, 0, 0,               // color bits ignored
		0,                              // no alpha Buffer
		0,                              // shift bit ignored
		0,                              // no accumulation buffer
		0, 0, 0, 0,                     // accumulation bits ignored
		16,                             // 16-bit z-buffer
		0,                              // no stencil buffer
		0,                              // no auxiliary buffer
		PFD_MAIN_PLANE,                 // main drawing layer
		0,                              // reserved
		0, 0, 0                         // layer masks ignored
	};

	// setup window (DC, pixel format, RC)
	// note: we will setup standard OpenGL1 style context
	// even when asked for OpenGL3 context since OpenGL1 context is necessary
	// for creating OpenGL3 style context
	GLuint pixelFormat;
	if(!(hDC=GetDC(glWindow)) ||
	   !(pixelFormat=ChoosePixelFormat(hDC,&pfd)) ||
	   !SetPixelFormat(hDC,pixelFormat,&pfd) ||
	   !(hRC=wglCreateContext(hDC)) ||
	   !wglMakeCurrent(hDC,hRC))
		throw std::runtime_error("Can not initialize OpenGL window.");

	// show window
	ShowWindow(window,SW_SHOWDEFAULT);

	// create surface
	surface=instance->createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window));

#else

	// open X connection
	display=XOpenDisplay(nullptr);
	if(display==nullptr)
		throw std::runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");

	// create window
	int blackColor=BlackPixel(display,DefaultScreen(display));
	Screen* screen=XDefaultScreenOfDisplay(display);
	uint32_t windowWidth=XWidthOfScreen(screen)/2;
	uint32_t windowHeight=XHeightOfScreen(screen)/2;
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowWidth,
		                        windowHeight,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Hello window!","Hello window!",None,NULL,0,NULL);
	wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteMessage,1);
	XMapWindow(display,window);

	// create surface
	surface=instance->createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window));

	// make sure GLX is supported
	if(!glXQueryExtension(display,nullptr,nullptr))
		throw std::runtime_error("X server has no OpenGL GLX extension");

	// choose visual for OpenGL
	struct XVisualInfoDeleter { void operator()(XVisualInfo* ptr) const { XFree(ptr); } };
	unique_ptr<XVisualInfo,XVisualInfoDeleter> vi(glXChooseVisual(display,DefaultScreen(display),array<int,14>{
		GLX_RGBA,GLX_RED_SIZE,1,GLX_GREEN_SIZE,1,GLX_BLUE_SIZE,1,GLX_ALPHA_SIZE,1,
		GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None,None}.data()
	));
	if(vi==nullptr)
		throw std::runtime_error("glXChooseVisual() failed.");

	// create GL context
	cx=glXCreateContext(display,vi.get(),0,GL_TRUE);
	if(cx==nullptr)
		throw std::runtime_error("glXCreateContext() failed.");

	// fill XSetWindowAttributes structure
	glWindowColormap=XCreateColormap(display,
	                                 RootWindow(display, DefaultScreen(display)),
	                                 vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.border_pixel=0;
	swa.event_mask=StructureNotifyMask;
	swa.override_redirect=False;
	swa.colormap=glWindowColormap;
	glWindow=XCreateWindow(display,DefaultRootWindow(display),
	                       0,0,windowWidth,windowHeight,
	                       0,vi->depth,InputOutput,vi->visual,
	                       CWBorderPixel|CWEventMask|CWOverrideRedirect|CWColormap,&swa);
	vi.reset();

	if(!glXMakeCurrent(display,glWindow,cx))
		throw std::runtime_error("glXMakeCurrent() failed.");

#endif

	// print OpenGL info
	cout<<"OpenGL renderer: "<<glGetString(GL_RENDERER)<<endl;
	cout<<"OpenGL version:  "<<glGetString(GL_VERSION)<<endl;
	cout<<"OpenGL vendor:   "<<glGetString(GL_VENDOR)<<endl;

	// OpenGL version
	float glVersion=[](){
		const char *ptr=(const char*)glGetString(GL_VERSION);
		while(*ptr!=0) {
			if(*ptr>='0' && *ptr<='9')
				return float(atof(ptr));
			++ptr;
		}
		return 0.0f;
	}();
	if(glVersion<4.5f)
		throw std::runtime_error("OpenGL version 4.5 or higher is required.");

	// get supported OpenGL extensions
	set<string> glExtensions;
	{
		PFNGLGETSTRINGIPROC glGetStringi=glGetProcAddress<PFNGLGETSTRINGIPROC>("glGetStringi");

		GLint numExtensions=0;
		glGetIntegerv(GL_NUM_EXTENSIONS,&numExtensions);
		for(int i=0; i<numExtensions; i++) {
			const GLubyte *exName = glGetStringi(GL_EXTENSIONS,i);
			if(exName)
				glExtensions.emplace((const char*)exName);
		}
	}

	// check presence of required OpenGL extensions
	if(glExtensions.find("GL_EXT_memory_object")==glExtensions.end())
		throw std::runtime_error("OpenGL does not support GL_EXT_memory_object extension.");
	if(glExtensions.find("GL_EXT_semaphore")==glExtensions.end())
		throw std::runtime_error("OpenGL does not support GL_EXT_semaphore extension.");
#ifdef _WIN32
	if(glExtensions.find("GL_EXT_semaphore_win32")==glExtensions.end())
		throw std::runtime_error("OpenGL does not support GL_EXT_semaphore_win32 extension.");
#else
	if(glExtensions.find("GL_EXT_semaphore_fd")==glExtensions.end())
		throw std::runtime_error("OpenGL does not support GL_EXT_semaphore_fd extension.");
#endif

	// OpenGL function pointers
	glCreateTextures=glGetProcAddress<PFNGLCREATETEXTURESPROC>("glCreateTextures");  // since OpenGL 4.5
	glCreateMemoryObjectsEXT=glGetProcAddress<PFNGLCREATEMEMORYOBJECTSEXTPROC>("glCreateMemoryObjectsEXT");  // GL_EXT_memory_object extension
	glTextureStorageMem2DEXT=glGetProcAddress<PFNGLTEXTURESTORAGEMEM2DEXTPROC>("glTextureStorageMem2DEXT");  // GL_EXT_memory_object extension
	glDeleteMemoryObjectsEXT=glGetProcAddress<PFNGLDELETEMEMORYOBJECTSEXTPROC>("glDeleteMemoryObjectsEXT");  // GL_EXT_memory_object extension
	glGenSemaphoresEXT=glGetProcAddress<PFNGLGENSEMAPHORESEXTPROC>("glGenSemaphoresEXT");  // GL_EXT_semaphore extension
	glDeleteSemaphoresEXT=glGetProcAddress<PFNGLDELETESEMAPHORESEXTPROC>("glDeleteSemaphoresEXT");  // GL_EXT_semaphore extension
	glWaitSemaphoreEXT=glGetProcAddress<PFNGLWAITSEMAPHOREEXTPROC>("glWaitSemaphoreEXT");  // GL_EXT_semaphore extension
	glSignalSemaphoreEXT=glGetProcAddress<PFNGLSIGNALSEMAPHOREEXTPROC>("glSignalSemaphoreEXT");  // GL_EXT_semaphore extension
	glGetUnsignedBytei_vEXT=glGetProcAddress<PFNGLGETUNSIGNEDBYTEI_VEXTPROC>("glGetUnsignedBytei_vEXT");  // GL_EXT_memory_object or GL_EXT_semaphore
#ifdef _WIN32
	glImportMemoryWin32HandleEXT=glGetProcAddress<PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC>("glImportMemoryWin32HandleEXT");  // GL_EXT_memory_object_win32 extension
	glImportSemaphoreWin32HandleEXT=glGetProcAddress<PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC>("glImportSemaphoreWin32HandleEXT");  // GL_EXT_semaphore_win32 extension
#else
	glImportMemoryFdEXT=glGetProcAddress<PFNGLIMPORTMEMORYFDEXTPROC>("glImportMemoryFdEXT");  // GL_EXT_memory_object_fd extension
	glImportSemaphoreFdEXT=glGetProcAddress<PFNGLIMPORTSEMAPHOREFDEXTPROC>("glImportSemaphoreFdEXT");  // GL_EXT_semaphore_fd extension
#endif
	glCreateFramebuffers=glGetProcAddress<PFNGLCREATEFRAMEBUFFERSPROC>("glCreateFramebuffers");  // since OpenGL 4.5
	glNamedFramebufferTexture=glGetProcAddress<PFNGLNAMEDFRAMEBUFFERTEXTUREPROC>("glNamedFramebufferTexture");  // since OpenGL 4.5
	glCheckNamedFramebufferStatus=glGetProcAddress<PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC>("glCheckNamedFramebufferStatus");  // since OpenGL 4.5
	glBindFramebuffer=glGetProcAddress<PFNGLBINDFRAMEBUFFERPROC>("glBindFramebuffer");  // since OpenGL 3.0
	glDeleteFramebuffers=glGetProcAddress<PFNGLDELETEFRAMEBUFFERSPROC>("glDeleteFramebuffers");  // since OpenGL 3.0
	glClipControl=glGetProcAddress<PFNGLCLIPCONTROLPROC>("glClipControl");  // since OpenGL 4.5

	// get OpenGL UUIDs
	vector<UUID> deviceUUIDsGL;
	UUID driverUUIDgl;
	{
		GLuint numDeviceUUIDs=0;
		glGetIntegerv(GL_NUM_DEVICE_UUIDS_EXT,reinterpret_cast<GLint*>(&numDeviceUUIDs));
		deviceUUIDsGL.resize(numDeviceUUIDs);
		for(GLuint i=0; i<numDeviceUUIDs; i++) {
			deviceUUIDsGL[i].fillByZeros();
			glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT,i,deviceUUIDsGL[i].data);
		}
		driverUUIDgl.fillByZeros();
		glGetUnsignedBytei_vEXT(GL_DRIVER_UUID_EXT,0,driverUUIDgl.data);
	}
	if(glGetError()!=GL_NO_ERROR)
		throw std::runtime_error("OpenGL error during initialization.");
	cout<<"OpenGL device UUID: ";
	for(size_t i=0,c=deviceUUIDsGL.size(); i<c; i++) {
		if(i!=0)  cout<<", ";
		cout<<deviceUUIDsGL[i].to_string();
	}
	cout<<endl;
	cout<<"OpenGL driver UUID: "<<driverUUIDgl.to_string()<<endl;

	// enumerate all physical devices
	cout<<"Vulkan devices:"<<endl;
	vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
	for(vk::PhysicalDevice pd:deviceList) {

		// print device name
		auto p=pd.getProperties2KHR<vk::PhysicalDeviceProperties2KHR,vk::PhysicalDeviceIDPropertiesKHR>(vkFuncs);
		cout<<"   "<<p.template get<vk::PhysicalDeviceProperties2KHR>().properties.deviceName<<endl;

		// print Vulkan device UUID
		UUID deviceUUIDvk=p.template get<vk::PhysicalDeviceIDPropertiesKHR>().deviceUUID;
		cout<<"       Device UUID: "<<deviceUUIDvk.to_string()<<endl;

		// print Vulkan driver UUID
		UUID driverUUIDvk=p.template get<vk::PhysicalDeviceIDPropertiesKHR>().driverUUID;
		cout<<"       Driver UUID: "<<driverUUIDvk.to_string()<<endl;

		// use physical device that matches UUIDs
		if(deviceUUIDsGL.size()==1 &&
			deviceUUIDsGL[0]==deviceUUIDvk &&
			driverUUIDgl==driverUUIDvk)
		{
			physicalDevice=pd;
		}
	}
	if(!physicalDevice)
		throw std::runtime_error("No Vulkan device found that is compatible with OpenGL context.");

	// get queue families
	std::tie(graphicsQueueFamily,presentationQueueFamily)=
		[](){
			uint32_t graphicsQueueFamily=UINT32_MAX;
			uint32_t presentationQueueFamily=UINT32_MAX;
			vector<vk::QueueFamilyProperties> queueFamilyList=physicalDevice.getQueueFamilyProperties();
			uint32_t i=0;
			for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
				bool p=physicalDevice.getSurfaceSupportKHR(i,surface.get())!=0;
				if(it->queueFlags&vk::QueueFlagBits::eGraphics) {
					if(p)
						return tuple<uint32_t,uint32_t>(i,i);
					if(graphicsQueueFamily==UINT32_MAX)
						graphicsQueueFamily=i;
				}
				else {
					if(p)
						if(presentationQueueFamily==UINT32_MAX)
							presentationQueueFamily=i;
				}
			}
			if(graphicsQueueFamily==UINT32_MAX||presentationQueueFamily==UINT32_MAX)
				throw std::runtime_error("No compatible devices.");
			return tuple<uint32_t,uint32_t>(graphicsQueueFamily,presentationQueueFamily);
		}();

	// report used device
	cout<<"Using Vulkan device:\n"
		   "   "<<physicalDevice.getProperties().deviceName<<endl;

	// create device
	device.reset(  // move assignment and physicalDevice.createDeviceUnique() does not work here because of bug in vulkan.hpp until VK_HEADER_VERSION 73 (bug was fixed on 2018-03-05 in vulkan.hpp git). Unfortunately, Ubuntu 18.04 carries still broken vulkan.hpp.
		physicalDevice.createDevice(
			vk::DeviceCreateInfo{
				vk::DeviceCreateFlags(),  // flags
				(graphicsQueueFamily==presentationQueueFamily)?uint32_t(1):uint32_t(2),  // queueCreateInfoCount
				array<const vk::DeviceQueueCreateInfo,2>{  // pQueueCreateInfos
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						graphicsQueueFamily,
						1,
						&(const float&)1.f,
					},
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						presentationQueueFamily,
						1,
						&(const float&)1.f,
					}
				}.data(),
				0,nullptr,  // no layers
				5,          // number of enabled extensions
				array<const char*,5>{
					"VK_KHR_swapchain",
					"VK_KHR_external_memory",
					"VK_KHR_external_semaphore",
#ifdef _WIN32
					"VK_KHR_external_memory_win32",
					"VK_KHR_external_semaphore_win32",
#else
					"VK_KHR_external_memory_fd",
					"VK_KHR_external_semaphore_fd",
#endif
				}.data(),  // enabled extension names
				nullptr,    // enabled features
			}
		)
	);

	// get function pointers
#if 0
	vkFuncs.vkGetPhysicalDeviceImageFormatProperties2KHR=PFN_vkGetPhysicalDeviceImageFormatProperties2KHR(instance->getProcAddr("vkGetPhysicalDeviceImageFormatProperties2KHR"));
#endif
#ifdef _WIN32
	vkFuncs.vkGetMemoryWin32HandleKHR=PFN_vkGetMemoryWin32HandleKHR(device->getProcAddr("vkGetMemoryWin32HandleKHR"));
	vkFuncs.vkGetSemaphoreWin32HandleKHR=PFN_vkGetSemaphoreWin32HandleKHR(device->getProcAddr("vkGetSemaphoreWin32HandleKHR"));
#else
	vkFuncs.vkGetMemoryFdKHR=PFN_vkGetMemoryFdKHR(device->getProcAddr("vkGetMemoryFdKHR"));
	vkFuncs.vkGetSemaphoreFdKHR=PFN_vkGetSemaphoreFdKHR(device->getProcAddr("vkGetSemaphoreFdKHR"));
#endif

	// get queues
	graphicsQueue=device->getQueue(graphicsQueueFamily,0);
	presentationQueue=device->getQueue(presentationQueueFamily,0);

	// choose surface format
	vector<vk::SurfaceFormatKHR> surfaceFormats=physicalDevice.getSurfaceFormatsKHR(surface.get());
	const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
	chosenSurfaceFormat=
		surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
			?wantedSurfaceFormat
			:std::find(surfaceFormats.begin(),surfaceFormats.end(),
			           wantedSurfaceFormat)!=surfaceFormats.end()
				?wantedSurfaceFormat
				:surfaceFormats[0];
	std::tie(depthFormatVk,depthFormatGL)=
		[](){
			constexpr array<std::tuple<vk::Format,GLenum>,3> formatList{
				std::make_tuple(vk::Format::eD32Sfloat,GL_DEPTH_COMPONENT32F),
				std::make_tuple(vk::Format::eD32SfloatS8Uint,GL_DEPTH32F_STENCIL8),
				std::make_tuple(vk::Format::eD24UnormS8Uint,GL_DEPTH24_STENCIL8)
			};
			for(auto f:formatList) {
				vk::FormatProperties p=physicalDevice.getFormatProperties(std::get<0>(f));
				if(p.optimalTilingFeatures&vk::FormatFeatureFlagBits::eDepthStencilAttachment)
					return f;
			}
			throw std::runtime_error("No suitable depth buffer format.");
		}();

	// render pass
	renderPass=
		device->createRenderPassUnique(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				2,                            // attachmentCount
				array<const vk::AttachmentDescription,2>{  // pAttachments
					vk::AttachmentDescription{  // color attachment
						vk::AttachmentDescriptionFlags(),  // flags
						chosenSurfaceFormat.format,        // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eStore,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::ePresentSrcKHR    // finalLayout
					},
					vk::AttachmentDescription{  // depth attachment
						vk::AttachmentDescriptionFlags(),  // flags
						depthFormatVk,                     // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eDontCare,  // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // finalLayout
					}
				}.data(),
				1,  // subpassCount
				&(const vk::SubpassDescription&)vk::SubpassDescription(  // pSubpasses
					vk::SubpassDescriptionFlags(),     // flags
					vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
					0,        // inputAttachmentCount
					nullptr,  // pInputAttachments
					1,        // colorAttachmentCount
					&(const vk::AttachmentReference&)vk::AttachmentReference(  // pColorAttachments
						0,  // attachment
						vk::ImageLayout::eColorAttachmentOptimal  // layout
					),
					nullptr,  // pResolveAttachments
					&(const vk::AttachmentReference&)vk::AttachmentReference(  // pDepthStencilAttachment
						1,  // attachment
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // layout
					),
					0,        // preserveAttachmentCount
					nullptr   // pPreserveAttachments
				),
				1,  // dependencyCount
				&(const vk::SubpassDependency&)vk::SubpassDependency(  // pDependencies
					VK_SUBPASS_EXTERNAL,   // srcSubpass
					0,                     // dstSubpass
					vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // srcStageMask
					vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // dstStageMask
					vk::AccessFlags(),     // srcAccessMask
					vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentRead|vk::AccessFlagBits::eColorAttachmentWrite),  // dstAccessMask
					vk::DependencyFlags()  // dependencyFlags
				)
			)
		);

	// create shader modules
	vsTwoTrianglesModule=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(vsTwoTrianglesSpirv),  // codeSize
			vsTwoTrianglesSpirv  // pCode
		)
	);
	fsTwoTrianglesModule=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(fsTwoTrianglesSpirv),  // codeSize
			fsTwoTrianglesSpirv  // pCode
		)
	);
	vsMergeModule=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(vsMergeSpirv),  // codeSize
			vsMergeSpirv  // pCode
		)
	);
	fsMergeModule=device->createShaderModuleUnique(
		vk::ShaderModuleCreateInfo(
			vk::ShaderModuleCreateFlags(),  // flags
			sizeof(fsMergeSpirv),  // codeSize
			fsMergeSpirv  // pCode
		)
	);

	// pipeline cache
	pipelineCache=device->createPipelineCacheUnique(
		vk::PipelineCacheCreateInfo(
			vk::PipelineCacheCreateFlags(),  // flags
			0,       // initialDataSize
			nullptr  // pInitialData
		)
	);

	// pipeline layout
	twoTrianglesPipelineLayout=device->createPipelineLayoutUnique(
		vk::PipelineLayoutCreateInfo{
			vk::PipelineLayoutCreateFlags(),  // flags
			0,       // setLayoutCount
			nullptr, // pSetLayouts
			0,       // pushConstantRangeCount
			nullptr  // pPushConstantRanges
		}
	);
	mergeDescriptorSetLayout=device->createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo{
			vk::DescriptorSetLayoutCreateFlags(),  // flags
			2,  // bindingCount
			array<const vk::DescriptorSetLayoutBinding,2>{  // pBindings
				vk::DescriptorSetLayoutBinding{
					0,       // binding
					vk::DescriptorType::eCombinedImageSampler,  // descriptorType
					1,       // descriptorCount
					vk::ShaderStageFlagBits::eFragment,  // stageFlags
					nullptr  // pImmutableSamplers
				},
				vk::DescriptorSetLayoutBinding{
					1,       // binding
					vk::DescriptorType::eCombinedImageSampler,  // descriptorType
					1,       // descriptorCount
					vk::ShaderStageFlagBits::eFragment,  // stageFlags
					nullptr  // pImmutableSamplers
				}
			}.data()
		}
	);
	mergePipelineLayout=device->createPipelineLayoutUnique(
		vk::PipelineLayoutCreateInfo{
			vk::PipelineLayoutCreateFlags(),  // flags
			1,       // setLayoutCount
			&mergeDescriptorSetLayout.get(), // pSetLayouts
			0,       // pushConstantRangeCount
			nullptr  // pPushConstantRanges
		}
	);

	// descriptor pool
	mergeDescriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo{
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				&(const vk::DescriptorPoolSize&)vk::DescriptorPoolSize{  // pPoolSizes
					vk::DescriptorType::eCombinedImageSampler,  // type
					2  // descriptorCount
				}
			}
		);

	// merge descriptor set
	mergeDescriptorSet=std::move(
		device->allocateDescriptorSetsUnique(
			vk::DescriptorSetAllocateInfo{
				mergeDescriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&mergeDescriptorSetLayout.get()  // pSetLayouts
			}
		)[0]);

	// texture sampler
	sampler=
		device->createSamplerUnique(
			vk::SamplerCreateInfo{
				vk::SamplerCreateFlags(),  // flags
				vk::Filter::eNearest,      // magFilter
				vk::Filter::eNearest,      // minFilter
				vk::SamplerMipmapMode::eNearest,  // mipmapMode
				vk::SamplerAddressMode::eClampToEdge,  // addressModeU
				vk::SamplerAddressMode::eClampToEdge,  // addressModeV
				vk::SamplerAddressMode::eClampToEdge,  // addressModeW
				0.f,       // mipLodBias
				VK_FALSE,  // anisotropyEnable
				0.f,       // maxAnisotropy
				VK_FALSE,  // compareEnable
				vk::CompareOp::eAlways,  // compareOp
				0.f,  // minLod
				0.f,  // maxLod
				vk::BorderColor::eFloatTransparentBlack,  // borderColor
				VK_FALSE  // unnormalizedCoordinates
			}
		);

	// command pool
	commandPool=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlags(),  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);

	// semaphores
	acquireCompleteSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderFinishedSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
}


/// Recreate swapchain and pipeline. The function is usually used on each window resize event and on application start.
static bool recreateSwapchainAndPipeline()
{
	// stop device and clear resources
	device->waitIdle();
	commandBuffers.clear();
	framebuffers.clear();
	depthImage.reset();
	depthImageMemory.reset();
	depthImageView.reset();
	twoTrianglesPipeline.reset();
	mergePipeline.reset();
	swapchainImageViews.clear();

	// currentSurfaceExtent
recreateSwapchain:
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	vk::Extent2D currentSurfaceExtent=(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
			?surfaceCapabilities.currentExtent
			:vk::Extent2D{max(min(windowWidth,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
			              max(min(windowHeight,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

	// avoid 0,0 surface extent
	// by waiting for valid window size
	// (0,0 is returned on some platforms (for instance Windows) when window is minimized.
	// The creation of swapchain then raises an exception, for instance vk::Result::eErrorOutOfDeviceMemory on Windows)
#ifdef _WIN32
	// run Win32 event loop
	if(currentSurfaceExtent.width==0||currentSurfaceExtent.height==0) {
		// wait for and process a message
		MSG msg;
		if(GetMessage(&msg,NULL,0,0)<=0)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		// process remaining messages
		while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
			if(msg.message==WM_QUIT)
				return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// try to recreate swapchain again
		goto recreateSwapchain;
	}
#else
	// run Xlib event loop
	if(currentSurfaceExtent.width==0||currentSurfaceExtent.height==0) {
		// process messages
		XEvent e;
		while(XPending(display)>0) {
			XNextEvent(display,&e);
			if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
				return false;
		}
		// try to recreate swapchain again
		goto recreateSwapchain;
	}
#endif

	// create swapchain
	swapchain=
		device->createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),   // flags
				surface.get(),                   // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.maxImageCount,surfaceCapabilities.minImageCount+1),
				chosenSurfaceFormat.format,      // imageFormat
				chosenSurfaceFormat.colorSpace,  // imageColorSpace
				currentSurfaceExtent,  // imageExtent
				1,  // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(graphicsQueueFamily==presentationQueueFamily)?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent, // imageSharingMode
				(graphicsQueueFamily==presentationQueueFamily)?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
				(graphicsQueueFamily==presentationQueueFamily)?nullptr:array<uint32_t,2>{graphicsQueueFamily,presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				[](vector<vk::PresentModeKHR>&& modes){  // presentMode
						return find(modes.begin(),modes.end(),vk::PresentModeKHR::eMailbox)!=modes.end()
							?vk::PresentModeKHR::eMailbox
							:vk::PresentModeKHR::eFifo; // fifo is guaranteed to be supported
					}(physicalDevice.getSurfacePresentModesKHR(surface.get())),
				VK_TRUE,         // clipped
				swapchain.get()  // oldSwapchain
			)
		);

	// swapchain images and image views
	vector<vk::Image> swapchainImages=device->getSwapchainImagesKHR(swapchain.get());
	swapchainImageViews.reserve(swapchainImages.size());
	for(vk::Image image:swapchainImages)
		swapchainImageViews.emplace_back(
			device->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image,                       // image
					vk::ImageViewType::e2D,      // viewType
					chosenSurfaceFormat.format,  // format
					vk::ComponentMapping(),      // components
					vk::ImageSubresourceRange(   // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1   // layerCount
					)
				)
			)
		);

	// depth image
	depthImage=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormatVk,           // format
				vk::Extent3D(currentSurfaceExtent.width,currentSurfaceExtent.height,1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eDepthStencilAttachment,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

	// depth image memory
	depthImageMemory=
		device->allocateMemoryUnique(
			[](vk::Image depthImage)->vk::MemoryAllocateInfo{
				vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(depthImage);
				vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
				for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
					if(memoryRequirements.memoryTypeBits&(1<<i))
						if(memoryProperties.memoryTypes[i].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal)
							return vk::MemoryAllocateInfo(
									memoryRequirements.size,  // allocationSize
									i                         // memoryTypeIndex
								);
				throw std::runtime_error("No suitable memory type found for depth buffer.");
			}(depthImage.get())
		);
	device->bindImageMemory(
		depthImage.get(),  // image
		depthImageMemory.get(),  // memory
		0  // memoryOffset
	);

	// depth image view
	depthImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				depthImage.get(),            // image
				vk::ImageViewType::e2D,      // viewType
				depthFormatVk,               // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eDepth,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);

	// depth image layout
	vk::UniqueCommandBuffer cb=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPool.get(),  // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1  // commandBufferCount
			)
		)[0]);
	cb->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);
	cb->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
		vk::PipelineStageFlagBits::eEarlyFragmentTests,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		0,nullptr,  // memoryBarrierCount,pMemoryBarriers
		0,nullptr,  // bufferMemoryBarrierCount,pBufferMemoryBarriers
		1,  // imageMemoryBarrierCount
		&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(  // pImageMemoryBarriers
			vk::AccessFlags(),  // srcAccessMask
			vk::AccessFlagBits::eDepthStencilAttachmentRead|vk::AccessFlagBits::eDepthStencilAttachmentWrite,  // dstAccessMask
			vk::ImageLayout::eUndefined,  // oldLayout
			vk::ImageLayout::eDepthStencilAttachmentOptimal,  // newLayout
			VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
			depthImage.get(),  // image
			vk::ImageSubresourceRange(  // subresourceRange
				(depthFormatVk==vk::Format::eD32Sfloat)  // aspectMask
					?vk::ImageAspectFlagBits::eDepth
					:vk::ImageAspectFlagBits::eDepth|vk::ImageAspectFlagBits::eStencil,
				0,  // baseMipLevel
				1,  // levelCount
				0,  // baseArrayLayer
				1   // layerCount
			)
		)
	);
	cb->end();
	graphicsQueue.submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				0,nullptr,nullptr,  // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
				1,&cb.get(),        // commandBufferCount,pCommandBuffers
				0,nullptr           // signalSemaphoreCount,pSignalSemaphores
			)
		),
		vk::Fence(nullptr)
	);
	graphicsQueue.waitIdle();

	// pipeline
	twoTrianglesPipeline=
		device->createGraphicsPipelineUnique(
			pipelineCache.get(),
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags
				2,  // stageCount
				array<const vk::PipelineShaderStageCreateInfo,2>{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,      // stage
						vsTwoTrianglesModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eFragment,    // stage
						fsTwoTrianglesModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					}
				}.data(),
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					0,        // vertexBindingDescriptionCount
					nullptr,  // pVertexBindingDescriptions
					0,        // vertexAttributeDescriptionCount
					nullptr   // pVertexAttributeDescriptions
				},
				&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
					vk::PipelineInputAssemblyStateCreateFlags(),  // flags
					vk::PrimitiveTopology::eTriangleList,  // topology
					VK_FALSE  // primitiveRestartEnable
				},
				nullptr, // pTessellationState
				&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
					vk::PipelineViewportStateCreateFlags(),  // flags
					1,  // viewportCount
					&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(currentSurfaceExtent.width),float(currentSurfaceExtent.height),0.f,1.f),  // pViewports
					1,  // scissorCount
					&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent)  // pScissors
				},
				&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),  // flags
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eNone,  // cullMode
					vk::FrontFace::eCounterClockwise,  // frontFace
					VK_FALSE,  // depthBiasEnable
					0.f,  // depthBiasConstantFactor
					0.f,  // depthBiasClamp
					0.f,  // depthBiasSlopeFactor
					1.f   // lineWidth
				},
				&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
					vk::PipelineMultisampleStateCreateFlags(),  // flags
					vk::SampleCountFlagBits::e1,  // rasterizationSamples
					VK_FALSE,  // sampleShadingEnable
					0.f,       // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				},
				&(const vk::PipelineDepthStencilStateCreateInfo&)vk::PipelineDepthStencilStateCreateInfo{  // pDepthStencilState
					vk::PipelineDepthStencilStateCreateFlags(),  // flags
					VK_TRUE,  // depthTestEnable
					VK_TRUE,  // depthWriteEnable
					vk::CompareOp::eLess,  // depthCompareOp
					VK_FALSE,  // depthBoundsTestEnable
					VK_FALSE,  // stencilTestEnable
					vk::StencilOpState(),  // front
					vk::StencilOpState(),  // back
					0.f,  // minDepthBounds
					0.f   // maxDepthBounds
				},
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					1,  // attachmentCount
					&(const vk::PipelineColorBlendAttachmentState&)vk::PipelineColorBlendAttachmentState{  // pAttachments
						VK_FALSE,  // blendEnable
						vk::BlendFactor::eZero,  // srcColorBlendFactor
						vk::BlendFactor::eZero,  // dstColorBlendFactor
						vk::BlendOp::eAdd,       // colorBlendOp
						vk::BlendFactor::eZero,  // srcAlphaBlendFactor
						vk::BlendFactor::eZero,  // dstAlphaBlendFactor
						vk::BlendOp::eAdd,       // alphaBlendOp
						vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|
							vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA  // colorWriteMask
					},
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				},
				nullptr,  // pDynamicState
				twoTrianglesPipelineLayout.get(),  // layout
				renderPass.get(),  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		);
	mergePipeline=
		device->createGraphicsPipelineUnique(
			pipelineCache.get(),
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags
				2,  // stageCount
				array<const vk::PipelineShaderStageCreateInfo,2>{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,      // stage
						vsMergeModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eFragment,    // stage
						fsMergeModule.get(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					}
				}.data(),
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					0,        // vertexBindingDescriptionCount
					nullptr,  // pVertexBindingDescriptions
					0,        // vertexAttributeDescriptionCount
					nullptr   // pVertexAttributeDescriptions
				},
				&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
					vk::PipelineInputAssemblyStateCreateFlags(),  // flags
					vk::PrimitiveTopology::eTriangleStrip,  // topology
					VK_FALSE  // primitiveRestartEnable
				},
				nullptr, // pTessellationState
				&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
					vk::PipelineViewportStateCreateFlags(),  // flags
					1,  // viewportCount
					&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(currentSurfaceExtent.width),float(currentSurfaceExtent.height),0.f,1.f),  // pViewports
					1,  // scissorCount
					&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent)  // pScissors
				},
				&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),  // flags
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eNone,  // cullMode
					vk::FrontFace::eCounterClockwise,  // frontFace
					VK_FALSE,  // depthBiasEnable
					0.f,  // depthBiasConstantFactor
					0.f,  // depthBiasClamp
					0.f,  // depthBiasSlopeFactor
					1.f   // lineWidth
				},
				&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
					vk::PipelineMultisampleStateCreateFlags(),  // flags
					vk::SampleCountFlagBits::e1,  // rasterizationSamples
					VK_FALSE,  // sampleShadingEnable
					0.f,       // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				},
				&(const vk::PipelineDepthStencilStateCreateInfo&)vk::PipelineDepthStencilStateCreateInfo{  // pDepthStencilState
					vk::PipelineDepthStencilStateCreateFlags(),  // flags
					VK_TRUE,  // depthTestEnable
					VK_TRUE,  // depthWriteEnable
					vk::CompareOp::eLess,  // depthCompareOp
					VK_FALSE,  // depthBoundsTestEnable
					VK_FALSE,  // stencilTestEnable
					vk::StencilOpState(),  // front
					vk::StencilOpState(),  // back
					0.f,  // minDepthBounds
					0.f   // maxDepthBounds
				},
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					1,  // attachmentCount
					&(const vk::PipelineColorBlendAttachmentState&)vk::PipelineColorBlendAttachmentState{  // pAttachments
						VK_FALSE,  // blendEnable
						vk::BlendFactor::eZero,  // srcColorBlendFactor
						vk::BlendFactor::eZero,  // dstColorBlendFactor
						vk::BlendOp::eAdd,       // colorBlendOp
						vk::BlendFactor::eZero,  // srcAlphaBlendFactor
						vk::BlendFactor::eZero,  // dstAlphaBlendFactor
						vk::BlendOp::eAdd,       // alphaBlendOp
						vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|
							vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA  // colorWriteMask
					},
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				},
				nullptr,  // pDynamicState
				mergePipelineLayout.get(),  // layout
				renderPass.get(),  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		);

	// framebuffers
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0,c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),   // flags
					renderPass.get(),               // renderPass
					2,  // attachmentCount
					array<vk::ImageView,2>{  // pAttachments
						swapchainImageViews[i].get(),
						depthImageView.get()
					}.data(),
					currentSurfaceExtent.width,     // width
					currentSurfaceExtent.height,    // height
					1  // layers
				)
			)
		);

	// shared color and depth image
	sharedColorImageVk=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::Extent3D(currentSurfaceExtent.width,currentSurfaceExtent.height,1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);
	sharedDepthImageVk=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormatVk,           // format
				vk::Extent3D(currentSurfaceExtent.width,currentSurfaceExtent.height,1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eDepthStencilAttachment|vk::ImageUsageFlagBits::eSampled,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

#if 0
	{
		array<vk::PhysicalDeviceImageFormatInfo2,2> formats{
			vk::PhysicalDeviceImageFormatInfo2{
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::ImageType::e2D,  // imageType
				vk::ImageTiling::eOptimal,  // tiling
				vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,  // usage
				vk::ImageCreateFlags()  // flags
			},
			vk::PhysicalDeviceImageFormatInfo2{
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::ImageType::e2D,  // imageType
				vk::ImageTiling::eOptimal,  // tiling
				vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,  // usage
				vk::ImageCreateFlags()  // flags
			}
		};
		vk::PhysicalDeviceExternalImageFormatInfo extraInfo(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
		formats[0].pNext=&extraInfo;
		formats[1].pNext=&extraInfo;
		vk::ImageFormatProperties2 imageFormatProperties2;
		vk::ExternalImageFormatProperties extraProperties;
		imageFormatProperties2.pNext=&extraProperties;

		for(size_t i=0; i<1; i++) {
			vk::Result r=physicalDevice.getImageFormatProperties2KHR(&formats[i],&imageFormatProperties2,vkFuncs);
			if(r==vk::Result::eErrorFormatNotSupported||
			   extraProperties.)

			cout<<"externalMemoryFeatures:        "<<(int&)extraProperties.externalMemoryProperties.externalMemoryFeatures<<endl;
			cout<<"exportFromImportedHandleTypes: "<<(int&)extraProperties.externalMemoryProperties.exportFromImportedHandleTypes<<endl;
			cout<<"compatibleHandleTypes:         "<<(int&)extraProperties.externalMemoryProperties.compatibleHandleTypes<<endl;
		}
	}
#endif

#if 0
	auto vkGetPhysicalDeviceImageFormatProperties2=PFN_vkGetPhysicalDeviceImageFormatProperties2(instance->getProcAddr("vkGetPhysicalDeviceImageFormatProperties2"));
	assert(vkGetPhysicalDeviceImageFormatProperties2);
	vk::PhysicalDeviceImageFormatInfo2 imageFormatInfo2{
		vk::Format::eR8G8B8A8Unorm,  // format
		vk::ImageType::e2D,  // imageType
		vk::ImageTiling::eOptimal,  // tiling
		vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,  // usage
		vk::ImageCreateFlags()  // flags
	};
	vk::PhysicalDeviceExternalImageFormatInfo extraInfo(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32);
	imageFormatInfo2.pNext=&extraInfo;
	vk::ImageFormatProperties2 imageFormatProperties2;
	vk::ExternalImageFormatProperties extraProperties;
	imageFormatProperties2.pNext=&extraProperties;
	vkGetPhysicalDeviceImageFormatProperties2(physicalDevice,
	                                          &(VkPhysicalDeviceImageFormatInfo2&)(imageFormatInfo2),
	                                          &(VkImageFormatProperties2&)imageFormatProperties2);
	cout<<"Win32 NT handle support: "<<(int&)extraProperties.externalMemoryProperties.externalMemoryFeatures<<endl;
	cout<<"Win32 NT handle support: "<<(int&)extraProperties.externalMemoryProperties.exportFromImportedHandleTypes<<endl;
	cout<<"Win32 NT handle support: "<<(int&)extraProperties.externalMemoryProperties.compatibleHandleTypes<<endl;
	extraInfo.handleType=vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt;
	vkGetPhysicalDeviceImageFormatProperties2(physicalDevice,
	                                          &(VkPhysicalDeviceImageFormatInfo2&)(imageFormatInfo2),
	                                          &(VkImageFormatProperties2&)imageFormatProperties2);
	cout<<"Win32 KMT handle support: "<<(int&)extraProperties.externalMemoryProperties.externalMemoryFeatures<<endl;
	cout<<"Win32 KMT handle support: "<<(int&)extraProperties.externalMemoryProperties.exportFromImportedHandleTypes<<endl;
	cout<<"Win32 KMT handle support: "<<(int&)extraProperties.externalMemoryProperties.compatibleHandleTypes<<endl;
#endif

	// shared color and depth image memory
	auto allocateMemory=
		[](vk::Image sharedImage){

			// find suitable memory type
			vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(sharedImage);
			vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
			for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
				if(memoryRequirements.memoryTypeBits&(1<<i))
					if(memoryProperties.memoryTypes[i].propertyFlags&vk::MemoryPropertyFlagBits::eDeviceLocal) {

						// allocate memory
						vk::MemoryAllocateInfo info(memoryRequirements.size,i);
					#ifdef _WIN32
						vk::ExportMemoryAllocateInfo exportInfo(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt);  // Kernel-Mode Thunk handle
					#else
						vk::ExportMemoryAllocateInfo exportInfo(vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd);
					#endif
						info.setPNext(&exportInfo);
						return make_tuple(device->allocateMemoryUnique(info),memoryRequirements.size);

					}
			throw std::runtime_error("No suitable memory type found.");
		};
	vk::DeviceSize sharedColorImageMemorySize;
	vk::DeviceSize sharedDepthImageMemorySize;
	std::tie(sharedColorImageMemoryVk,sharedColorImageMemorySize)=allocateMemory(sharedColorImageVk.get());
	std::tie(sharedDepthImageMemoryVk,sharedDepthImageMemorySize)=allocateMemory(sharedDepthImageVk.get());
#ifdef _WIN32
	struct UniqueHandle {
		HANDLE handle;
		inline UniqueHandle() : handle(INVALID_HANDLE_VALUE)  {}
		inline UniqueHandle(HANDLE h) : handle(h)  {}
		inline ~UniqueHandle()  { if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); }
	};
	HANDLE sharedColorImageMemoryWin32KmtHandle(
		device->getMemoryWin32HandleKHR(
			vk::MemoryGetWin32HandleInfoKHR{
				sharedColorImageMemoryVk.get(),  // memory
				vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt  // handleType
			},
			vkFuncs
		)
	);
	HANDLE sharedDepthImageMemoryWin32KmtHandle(
		device->getMemoryWin32HandleKHR(
			vk::MemoryGetWin32HandleInfoKHR{
				sharedDepthImageMemoryVk.get(),  // memory
				vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt  // handleType
			},
			vkFuncs
		)
	);
	if(sharedColorImageMemoryWin32KmtHandle==nullptr||sharedDepthImageMemoryWin32KmtHandle==nullptr)
		throw std::runtime_error("Can not allocate OpenGL-Vulkan shared handle.");
#else
	struct UniqueFd {
		int fd;
		inline UniqueFd() : fd(-1)  {}
		inline UniqueFd(int f) : fd(f)  {}
		inline ~UniqueFd()  { if(fd!=-1) close(fd); }
	};
	UniqueFd sharedColorImageMemoryFd(
		device->getMemoryFdKHR(
			vk::MemoryGetFdInfoKHR{
				sharedColorImageMemoryVk.get(),  // memory
				vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd  // handleType
			},
			vkFuncs
		)
	);
	UniqueFd sharedDepthImageMemoryFd(
		device->getMemoryFdKHR(
			vk::MemoryGetFdInfoKHR{
				sharedDepthImageMemoryVk.get(),  // memory
				vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd  // handleType
			},
			vkFuncs
		)
	);
#endif
	device->bindImageMemory(
		sharedColorImageVk.get(),        // image
		sharedColorImageMemoryVk.get(),  // memory
		0  // memoryOffset
	);
	device->bindImageMemory(
		sharedDepthImageVk.get(),        // image
		sharedDepthImageMemoryVk.get(),  // memory
		0  // memoryOffset
	);

	// sharedColorImageView, sharedDepthImageView
	sharedColorImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				sharedColorImageVk.get(),    // image
				vk::ImageViewType::e2D,      // viewType
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);
	sharedDepthImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				sharedDepthImageVk.get(),    // image
				vk::ImageViewType::e2D,      // viewType
				depthFormatVk,               // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eDepth,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);

	// create color and depth OpenGL textures
	glCreateTextures(GL_TEXTURE_2D,1,&sharedColorTextureGL.texture);
	glCreateTextures(GL_TEXTURE_2D,1,&sharedDepthTextureGL.texture);

	// create color and depth OpenGL memory objects
	glCreateMemoryObjectsEXT(1,&sharedColorTextureMemoryGL.memory);
	glCreateMemoryObjectsEXT(1,&sharedDepthTextureMemoryGL.memory);
#ifdef _WIN32
	// import Vulkan win32 kmt handle
	// (it does not transfers ownership of the handle,
	// the handle must be closed when not needed)
	glImportMemoryWin32HandleEXT(sharedColorTextureMemoryGL.memory,sharedColorImageMemorySize,GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT,sharedColorImageMemoryWin32KmtHandle);
	glImportMemoryWin32HandleEXT(sharedDepthTextureMemoryGL.memory,sharedDepthImageMemorySize,GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT,sharedDepthImageMemoryWin32KmtHandle);
#else
	// import Vulkan fd memory handle
	// (it transfers ownership of the file descriptor,
	// the file descriptor must not be manipulated or closed after the import)
	glImportMemoryFdEXT(sharedColorTextureMemoryGL.memory,sharedColorImageMemorySize,GL_HANDLE_TYPE_OPAQUE_FD_EXT,sharedColorImageMemoryFd.fd);
	glImportMemoryFdEXT(sharedDepthTextureMemoryGL.memory,sharedDepthImageMemorySize,GL_HANDLE_TYPE_OPAQUE_FD_EXT,sharedDepthImageMemoryFd.fd);
	sharedColorImageMemoryFd.fd=-1;
	sharedDepthImageMemoryFd.fd=-1;
#endif
	glTextureStorageMem2DEXT(sharedColorTextureGL.texture,1,GL_RGBA8,currentSurfaceExtent.width,currentSurfaceExtent.height,sharedColorTextureMemoryGL.memory,0);
	glTextureStorageMem2DEXT(sharedDepthTextureGL.texture,1,depthFormatGL,currentSurfaceExtent.width,currentSurfaceExtent.height,sharedDepthTextureMemoryGL.memory,0);

	// create Vulkan semaphores
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo;
		vk::ExportSemaphoreCreateInfo exportInfo;
		semaphoreCreateInfo.pNext=&exportInfo;
#ifdef _WIN32
		exportInfo.handleTypes=vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt;
#else
		exportInfo.handleTypes=vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
#endif
		glStartSemaphoreVk=device->createSemaphoreUnique(semaphoreCreateInfo);
		glDoneSemaphoreVk=device->createSemaphoreUnique(semaphoreCreateInfo);
	}

	// create OpenGL semaphores
	glGenSemaphoresEXT(1,&glStartSemaphoreGL.semaphore);
	glGenSemaphoresEXT(1,&glDoneSemaphoreGL.semaphore);

	// import semaphores to OpenGL
#ifdef _WIN32
	// import semaphores
	// (it does not transfers ownership of the handle,
	// the handle must be closed when not needed)
	HANDLE h1(
		device->getSemaphoreWin32HandleKHR(  // handle
			vk::SemaphoreGetWin32HandleInfoKHR(  // getWin32HandleInfo
				glStartSemaphoreVk.get(),  // semaphore
				vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt  // handleType
			),
			vkFuncs
		)
	);
	glImportSemaphoreWin32HandleEXT(
		glStartSemaphoreGL.semaphore,  // semaphore
		GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT,  // handleType
		h1  // handle
	);
	HANDLE h2(
		device->getSemaphoreWin32HandleKHR(  // handle
			vk::SemaphoreGetWin32HandleInfoKHR(  // getWin32HandleInfo
				glDoneSemaphoreVk.get(),  // semaphore
				vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt  // handleType
			),
			vkFuncs
		)
	);
	glImportSemaphoreWin32HandleEXT(
		glDoneSemaphoreGL.semaphore,  // semaphore
		GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT,  // handleType
		h2  // handle
	);
#else
	// import semaphores
	// (it transfers ownership of the file descriptor,
	// the file descriptor must not be manipulated or closed after the import)
	glImportSemaphoreFdEXT(
		glStartSemaphoreGL.semaphore,
		GL_HANDLE_TYPE_OPAQUE_FD_EXT,
		device->getSemaphoreFdKHR(
			vk::SemaphoreGetFdInfoKHR(
				glStartSemaphoreVk.get(),
				vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd
			),
			vkFuncs
		)
	);
	glImportSemaphoreFdEXT(
		glDoneSemaphoreGL.semaphore,
		GL_HANDLE_TYPE_OPAQUE_FD_EXT,
		device->getSemaphoreFdKHR(
			vk::SemaphoreGetFdInfoKHR(
				glDoneSemaphoreVk.get(),
				vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd
			),
			vkFuncs
		)
	);
#endif

	// create transition command buffer
	transitionCommandBuffer=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPool.get(),  // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1  // commandBufferCount
			)
		)[0]);
	transitionCommandBuffer->begin(vk::CommandBufferBeginInfo{});
	transitionCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
		vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eEarlyFragmentTests|  // dstStageMask
			vk::PipelineStageFlagBits::eLateFragmentTests,
		vk::DependencyFlags(),  // dependencyFlags
		nullptr,  // memoryBarriers
		nullptr,  // bufferMemoryBarriers
		vk::ArrayProxy<const vk::ImageMemoryBarrier>{
			vk::ImageMemoryBarrier{  // imageMemoryBarriers
				vk::AccessFlags(),                          // srcAccessMask
				vk::AccessFlagBits::eColorAttachmentWrite,  // dstAccessMask
				vk::ImageLayout::eUndefined,                // oldLayout
				vk::ImageLayout::eColorAttachmentOptimal,   // newLayout
				0,                          // srcQueueFamilyIndex
				0,                          // dstQueueFamilyIndex
				sharedColorImageVk.get(),   // image
				vk::ImageSubresourceRange{  // subresourceRange
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				}
			},
			vk::ImageMemoryBarrier{  // imageMemoryBarriers
				vk::AccessFlags(),                                // srcAccessMask
				vk::AccessFlagBits::eDepthStencilAttachmentRead|  // dstAccessMask
					vk::AccessFlagBits::eDepthStencilAttachmentWrite,
				vk::ImageLayout::eUndefined,                      // oldLayout
				vk::ImageLayout::eDepthStencilAttachmentOptimal,  // newLayout
				0,                          // srcQueueFamilyIndex
				0,                          // dstQueueFamilyIndex
				sharedDepthImageVk.get(),   // image
				vk::ImageSubresourceRange{  // subresourceRange
					vk::ImageAspectFlagBits::eDepth,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				}
			}
		}
	);
	transitionCommandBuffer->end();

	// setup OpenGL framebuffer
	glCreateFramebuffers(1,&glFramebuffer.framebuffer);
	glNamedFramebufferTexture(glFramebuffer.framebuffer,GL_COLOR_ATTACHMENT0,sharedColorTextureGL.texture,0);
	glNamedFramebufferTexture(glFramebuffer.framebuffer,GL_DEPTH_ATTACHMENT,sharedDepthTextureGL.texture,0);
	if(glCheckNamedFramebufferStatus(glFramebuffer.framebuffer,GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("Framebuffer incomplete error.");
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER,glFramebuffer.framebuffer);
	glEnable(GL_DEPTH_TEST);
	glViewport(0,0,currentSurfaceExtent.width,currentSurfaceExtent.height);
	glClipControl(
		GL_UPPER_LEFT,  // invert final image's y axis to avoid doing it in fragment shader
		GL_ZERO_TO_ONE  // make NDC (Normalized Device Coordinates) z-axis from 0 to +1 instead of from -1 to +1 to match Vulkan coordinates and improve z-precision
	);
	if(glGetError()!=GL_NO_ERROR)
		throw std::runtime_error("OpenGL error during initialization.");

	// update descriptor set
	// (mergePipeline uses sampler2D and this one must be updated)
	device->updateDescriptorSets(
		2,  // descriptorWriteCount
		array<const vk::WriteDescriptorSet,2>{  // pDescriptorWrites
			vk::WriteDescriptorSet{
				mergeDescriptorSet.get(),  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				&(const vk::DescriptorImageInfo&)vk::DescriptorImageInfo{  // pImageInfo
					sampler.get(),          // sampler
					sharedColorImageView.get(),  // imageView
					vk::ImageLayout::eColorAttachmentOptimal  // imageLayout
				},
				nullptr,  // pBufferInfo
				nullptr   // pTexelBufferView
			},
			vk::WriteDescriptorSet{
				mergeDescriptorSet.get(),  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				&(const vk::DescriptorImageInfo&)vk::DescriptorImageInfo{  // pImageInfo
					sampler.get(),          // sampler
					sharedDepthImageView.get(),  // imageView
					vk::ImageLayout::eDepthStencilAttachmentOptimal  // imageLayout
				},
				nullptr,  // pBufferInfo
				nullptr   // pTexelBufferView
			}
		}.data(),
		0,       // descriptorCopyCount
		nullptr  // pDescriptorCopies
	);

	// reallocate command buffers
	if(commandBuffers.size()!=swapchainImages.size()) {
		commandBuffers=
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					uint32_t(swapchainImages.size())   // commandBufferCount
				)
			);
	}

	// record command buffers
	for(size_t i=0,c=swapchainImages.size(); i<c; i++) {
		vk::CommandBuffer& cb=commandBuffers[i].get();
		cb.begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
				nullptr  // pInheritanceInfo
			)
		);
		cb.beginRenderPass(
			vk::RenderPassBeginInfo(
				renderPass.get(),       // renderPass
				framebuffers[i].get(),  // framebuffer
				vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent),  // renderArea
				2,                      // clearValueCount
				array<vk::ClearValue,2>{  // pClearValues
					vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}),
					vk::ClearDepthStencilValue(1.f,0)
				}.data()
			),
			vk::SubpassContents::eInline
		);
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,twoTrianglesPipeline.get());
		cb.draw(6,1,0,0);  // draw two triangles
		cb.bindPipeline(vk::PipelineBindPoint::eGraphics,mergePipeline.get());
		cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,mergePipelineLayout.get(),0,mergeDescriptorSet.get(),nullptr);
		cb.draw(4,1,0,0);  // draw full-screen quad
		cb.endRenderPass();
		cb.end();
	}

	return true;
}


/// Queue one frame for rendering
static bool queueFrame()
{
	// acquire next image
	uint32_t imageIndex;
	vk::Result r=device->acquireNextImageKHR(swapchain.get(),numeric_limits<uint64_t>::max(),acquireCompleteSemaphore.get(),vk::Fence(nullptr),&imageIndex);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { if(!recreateSwapchainAndPipeline()) return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Device::acquireNextImageKHR");
	}

	// submit transition
	graphicsQueue.submit(
		vk::SubmitInfo(
			1,&acquireCompleteSemaphore.get(),  // waitSemaphoreCount,pWaitSemaphores
			&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eBottomOfPipe),  // pWaitDstStageMask
			1,&transitionCommandBuffer.get(),  // commandBufferCount,pCommandBuffers
			1,&glStartSemaphoreVk.get()        // signalSemaphoreCount,pSignalSemaphores
		),
		nullptr  // fence
	);

	// submit OpenGL work
	glWaitSemaphoreEXT(glStartSemaphoreGL.semaphore,  // semaphore
	                   0,nullptr,                     // numBufferBarriers,buffers
	                   2,array<GLuint,2>{        // numTextureBarriers,textures
		                   sharedColorTextureGL.texture,
		                   sharedDepthTextureGL.texture
	                   }.data(),
	                   array<GLenum,2>{          // srcLayouts
		                   GL_LAYOUT_COLOR_ATTACHMENT_EXT,
		                   GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT
	                   }.data());

	// render a triangle
	// coordinates are given in OpenGL NDC (Normalized Device Coordinates; x,y,z are in range -1,+1;
	// x points right, y points up, z points forward) 
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(0.f,0.f,1.f);
	glVertex3f(-0.5f,-0.8f, 1.0f);
	glColor3f(1.f,0.f,0.f);
	glVertex3f( 0.0f, 0.2f, 0.0f);
	glColor3f(0.f,1.f,0.f);
	glVertex3f( 0.5f,-0.8f, 1.0f);
	glEnd();

	glSignalSemaphoreEXT(glDoneSemaphoreGL.semaphore,  // semaphore
	                     0,nullptr,                    // numBufferBarriers+buffers
	                     2,array<GLuint,2>{       // numTextureBarriers,textures
		                     sharedColorTextureGL.texture,
		                     sharedDepthTextureGL.texture
	                     }.data(),
	                     array<GLenum,2>{         // dstLayouts
		                     GL_LAYOUT_SHADER_READ_ONLY_EXT,
		                     GL_LAYOUT_SHADER_READ_ONLY_EXT
	                     }.data());
	glFlush();  // it is important to flush OpenGL

	// submit Vulkan work
	graphicsQueue.submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				1,                                  // waitSemaphoreCount
				&glDoneSemaphoreVk.get(),           // pWaitSemaphores
				&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
				1,&commandBuffers[imageIndex].get(),  // commandBufferCount+pCommandBuffers
				1,&renderFinishedSemaphore.get()      // signalSemaphoreCount+pSignalSemaphores
			)
		),
		nullptr  // fence
	);

	// submit image for presentation
	r=presentationQueue.presentKHR(
		&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
			1,&renderFinishedSemaphore.get(),  // waitSemaphoreCount+pWaitSemaphores
			1,&swapchain.get(),                // swapchainCount+pSwapchains
			&imageIndex,       // pImageIndices
			nullptr            // pResults
		)
	);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { if(!recreateSwapchainAndPipeline()) return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Queue::presentKHR");
	}

	// return success
	return true;
}


/// main function of the application
int main(int,char**)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// init Vulkan and open window
		init();

		// create swapchain and pipeline
		if(!recreateSwapchainAndPipeline())
			goto ExitMainLoop;

#ifdef _WIN32

		// run Win32 event loop
		MSG msg;
		while(true){

			// process messages
			while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
				if(msg.message==WM_QUIT)
					goto ExitMainLoop;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

#else

		// run Xlib event loop
		XEvent e;
		while(true) {

			// process messages
			while(XPending(display)>0) {
				XNextEvent(display,&e);
				if(e.type==ClientMessage&&ulong(e.xclient.data.l[0])==wmDeleteMessage)
					goto ExitMainLoop;
			}

#endif

			// queue frame
			if(!queueFrame())
				goto ExitMainLoop;

			// wait for rendering to complete
			presentationQueue.waitIdle();
		}
	ExitMainLoop:
		device->waitIdle();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
