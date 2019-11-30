#pragma once

#include <list>
#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <CadR/AllocationManagers.h>
#include <CadR/Export.h>

namespace CadR {

class AttribSizeList;
class AttribStorage;
class DrawCommand;
class Mesh;
class StagingBuffer;
class VulkanDevice;
class VulkanInstance;
struct DrawCommandGpuData;
struct PrimitiveSetGpuData;


class Renderer final {
private:

	VulkanDevice* _device;
	uint32_t _graphicsQueueFamily;
	vk::Queue _graphicsQueue;
	vk::PhysicalDeviceMemoryProperties _memoryProperties;

	std::map<AttribSizeList,std::list<AttribStorage>> _attribStorages;
	AttribStorage*   _emptyStorage;
	vk::Buffer       _indexBuffer;
	vk::DeviceMemory _indexBufferMemory;
	vk::Buffer       _primitiveSetBuffer;
	vk::DeviceMemory _primitiveSetBufferMemory;
	vk::Buffer       _drawCommandBuffer;
	vk::DeviceMemory _drawCommandBufferMemory;
	vk::Buffer       _matrixListControlBuffer;
	vk::DeviceMemory _matrixListControlBufferMemory;
	vk::Buffer       _drawIndirectBuffer;
	vk::DeviceMemory _drawIndirectBufferMemory;
	vk::Buffer       _stateSetBuffer;
	vk::DeviceMemory _stateSetBufferMemory;
	ArrayAllocationManager<Mesh> _indexAllocationManager;  ///< Allocation manager for index data.
	ArrayAllocationManager<Mesh> _primitiveSetAllocationManager;  ///< Allocation manager for primitiveSet data.
	ItemAllocationManager        _drawCommandAllocationManager;
	vk::CommandPool _stateSetCommandPool;

	vk::CommandPool _transientCommandPool;
	vk::CommandBuffer _uploadingCommandBuffer;
	std::list<std::tuple<vk::Buffer,vk::DeviceMemory>> _objectsToDeleteAfterCopyOperation;

	vk::ShaderModule _processDrawCommandsShader;
	vk::PipelineCache _pipelineCache;
	vk::Pipeline _processDrawCommandsPipeline;

	static Renderer* _instance;

public:

	CADR_EXPORT static Renderer* get();
	CADR_EXPORT static void set(Renderer* r);

	CADR_EXPORT Renderer() = delete;
	CADR_EXPORT Renderer(VulkanDevice* device,VulkanInstance* instance,vk::PhysicalDevice physicalDevice,
	                     uint32_t graphicsQueueFamily);
	CADR_EXPORT ~Renderer();

	CADR_EXPORT VulkanDevice* device() const;
	CADR_EXPORT uint32_t graphicsQueueFamily() const;
	CADR_EXPORT vk::Queue graphicsQueue() const;
	CADR_EXPORT const vk::PhysicalDeviceMemoryProperties& memoryProperties() const;

	CADR_EXPORT vk::Buffer indexBuffer() const;
	CADR_EXPORT vk::Buffer primitiveSetBuffer() const;
	CADR_EXPORT vk::Buffer drawCommandBuffer() const;
	CADR_EXPORT vk::Buffer matrixListControlBuffer() const;
	CADR_EXPORT vk::CommandPool stateSetCommandPool() const;
	CADR_EXPORT vk::PipelineCache pipelineCache() const;

	CADR_EXPORT AttribStorage* getOrCreateAttribStorage(const AttribSizeList& attribSizeList);
	CADR_EXPORT std::map<AttribSizeList,std::list<AttribStorage>>& getAttribStorages();
	CADR_EXPORT const AttribStorage* emptyStorage() const;
	CADR_EXPORT AttribStorage* emptyStorage();

	CADR_EXPORT vk::DeviceMemory allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags);
	CADR_EXPORT vk::CommandBuffer uploadingCommandBuffer() const;
	CADR_EXPORT void scheduleCopyOperation(StagingBuffer& stagingBuffer);
	CADR_EXPORT void executeCopyOperations();

	CADR_EXPORT const ArrayAllocation<Mesh>& indexAllocation(uint32_t id) const;  ///< Returns index allocation for particular id.
	CADR_EXPORT ArrayAllocation<Mesh>& indexAllocation(uint32_t id);   ///< Returns index allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<Mesh>& indexAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<Mesh>& indexAllocationManager();

	CADR_EXPORT void uploadIndices(Mesh& m,std::vector<uint32_t>&& indexData,size_t dstIndex=0);
	CADR_EXPORT StagingBuffer createIndexStagingBuffer(Mesh& m);
	CADR_EXPORT StagingBuffer createIndexStagingBuffer(Mesh& m,size_t firstIndex,size_t numIndices);

	CADR_EXPORT const ArrayAllocation<Mesh>& primitiveSetAllocation(uint32_t id) const;  ///< Returns primitiveSet allocation for particular id.
	CADR_EXPORT ArrayAllocation<Mesh>& primitiveSetAllocation(uint32_t id);   ///< Returns primitiveSet allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<Mesh>& primitiveSetAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<Mesh>& primitiveSetAllocationManager();

	CADR_EXPORT void uploadPrimitiveSets(Mesh& m,std::vector<PrimitiveSetGpuData>&& primitiveSetData,size_t dstPrimitiveSet=0);
	CADR_EXPORT StagingBuffer createPrimitiveSetStagingBuffer(Mesh& m);
	CADR_EXPORT StagingBuffer createPrimitiveSetStagingBuffer(Mesh& m,size_t firstPrimitiveSet,size_t numPrimitiveSets);

	CADR_EXPORT ItemAllocation*const& drawCommandAllocation(uint32_t id) const;  ///< Returns drawCommand allocation for particular id.
	CADR_EXPORT ItemAllocation*& drawCommandAllocation(uint32_t id);   ///< Returns drawCommand allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ItemAllocationManager& drawCommandAllocationManager() const;
	CADR_EXPORT ItemAllocationManager& drawCommandAllocationManager();

	CADR_EXPORT void uploadDrawCommand(DrawCommand& dc,const DrawCommandGpuData& drawCommandData);
	CADR_EXPORT StagingBuffer createDrawCommandStagingBuffer(DrawCommand& dc);

protected:
	CADR_EXPORT void purgeObjectsToDeleteAfterCopyOperation();
};


// inline methods
inline Renderer* Renderer::get()  { return _instance; }
inline void Renderer::set(Renderer* r)  { _instance=r; }
inline VulkanDevice* Renderer::device() const  { return _device; }
inline uint32_t Renderer::graphicsQueueFamily() const  { return _graphicsQueueFamily; }
inline vk::Queue Renderer::graphicsQueue() const  { return _graphicsQueue; }
inline const vk::PhysicalDeviceMemoryProperties& Renderer::memoryProperties() const  { return _memoryProperties; }
inline vk::Buffer Renderer::indexBuffer() const  { return _indexBuffer; }
inline vk::Buffer Renderer::primitiveSetBuffer() const  { return _primitiveSetBuffer; }
inline vk::Buffer Renderer::drawCommandBuffer() const  { return _drawCommandBuffer; }
inline vk::Buffer Renderer::matrixListControlBuffer() const  { return _matrixListControlBuffer; }
inline vk::CommandPool Renderer::stateSetCommandPool() const  { return _stateSetCommandPool; }
inline vk::PipelineCache Renderer::pipelineCache() const  { return _pipelineCache; }
inline std::map<AttribSizeList,std::list<AttribStorage>>& Renderer::getAttribStorages()  { return _attribStorages; }
inline const AttribStorage* Renderer::emptyStorage() const  { return _emptyStorage; }
inline AttribStorage* Renderer::emptyStorage()  { return _emptyStorage; }
inline const ArrayAllocation<Mesh>& Renderer::indexAllocation(uint32_t id) const  { return _indexAllocationManager[id]; }
inline ArrayAllocation<Mesh>& Renderer::indexAllocation(uint32_t id)  { return _indexAllocationManager[id]; }
inline const ArrayAllocationManager<Mesh>& Renderer::indexAllocationManager() const  { return _indexAllocationManager; }
inline ArrayAllocationManager<Mesh>& Renderer::indexAllocationManager()  { return _indexAllocationManager; }
inline const ArrayAllocation<Mesh>& Renderer::primitiveSetAllocation(uint32_t id) const  { return _primitiveSetAllocationManager[id]; }
inline ArrayAllocation<Mesh>& Renderer::primitiveSetAllocation(uint32_t id)  { return _primitiveSetAllocationManager[id]; }
inline const ArrayAllocationManager<Mesh>& Renderer::primitiveSetAllocationManager() const  { return _primitiveSetAllocationManager; }
inline ArrayAllocationManager<Mesh>& Renderer::primitiveSetAllocationManager()  { return _primitiveSetAllocationManager; }
inline ItemAllocation*const& Renderer::drawCommandAllocation(uint32_t id) const  { return _drawCommandAllocationManager[id]; }
inline ItemAllocation*& Renderer::drawCommandAllocation(uint32_t id)  { return _drawCommandAllocationManager[id]; }
inline const ItemAllocationManager& Renderer::drawCommandAllocationManager() const  { return _drawCommandAllocationManager; }
inline ItemAllocationManager& Renderer::drawCommandAllocationManager()  { return _drawCommandAllocationManager; }
inline vk::CommandBuffer Renderer::uploadingCommandBuffer() const  { return _uploadingCommandBuffer; }


}

#if 0
      class DrawCommandStorage : public BufferStorage<DrawCommandAllocationManager, // derived from ItemAllocationManager
            DrawCommandGpuData> {
      public:
#if _MSC_VER<1900
         // MSVC 2013 (tested with Update 4 and 5) fails to inherit constructors according to C++11 standard
         inline DrawCommandStorage(unsigned capacity=0,unsigned flags=0x88E8/*GL_DYNAMIC_DRAW*/,void *data=nullptr) : BufferStorage(capacity,flags,data) {}
         inline DrawCommandStorage(unsigned capacity,unsigned numNullItems,unsigned flags,void *data=nullptr) : BufferStorage(capacity,numNullItems,flags,data) {}
         inline DrawCommandStorage(unsigned bufferSize,unsigned allocManagerCapacity,unsigned numNullItems,unsigned flags,void *data=nullptr) : BufferStorage(bufferSize,allocManagerCapacity,numNullItems,flags,data) {}
#else
         using BufferStorage::BufferStorage;
#endif
         inline void alloc(DrawCommand* id)  { BufferStorage<DrawCommandAllocationManager,DrawCommandGpuData>::alloc(&id->data); }  ///< \brief Allocates one draw command and stores its index in the DrawCommand pointed by id parameter.
         inline void alloc(unsigned num,DrawCommand *ids)  { BufferStorage<DrawCommandAllocationManager,DrawCommandGpuData>::alloc(num,&ids->data); }  ///< \brief Allocates number of draw commands. Array pointed by ids must be at least num DrawCommands long.
      };
      typedef BufferStorage<ArrayAllocationManager<Mesh>,
            PrimitiveGpuData> PrimitiveStorage;
      typedef BufferStorage<ArrayAllocationManager<MatrixList>,
            MatrixGpuData> MatrixStorage;
      typedef BufferStorage<ItemAllocationManager,
            ListControlGpuData> ListControlStorage;
      typedef BufferStorage<ItemAllocationManager,
            StateSetGpuData> StateSetStorage;


      class GERG_EXPORT RenderingContext {
      public:

         typedef AttribConfig::InstanceList AttribConfigInstances;
         typedef std::vector<std::shared_ptr<Transformation>> TransformationGraphList;
         enum class MappedBufferAccess : uint8_t { READ=0x1, WRITE=0x2, READ_WRITE=0x3, NO_ACCESS=0x0 };

         ge::gl::Context gl;

      protected:

         mutable PrimitiveStorage _primitiveStorage;
         mutable DrawCommandStorage _drawCommandStorage;
         mutable MatrixStorage _matrixStorage;
         mutable ListControlStorage _matrixListControlStorage;
         mutable StateSetStorage _stateSetStorage;
         ItemAllocationManager _transformationAllocationManager;
         float *_cpuTransformationBuffer;
         std::shared_ptr<ge::gl::Buffer> _drawIndirectBuffer;

         AttribConfigInstances _attribConfigInstances;
         unsigned _numAttribStorages;
         std::shared_ptr<StateSetManager> _stateSetManager;
         std::map<std::string,std::weak_ptr<ge::gl::Texture>> _textureCache;
         TransformationGraphList _transformationGraphs;
         std::shared_ptr<MatrixList> _emptyMatrixList;
         bool _useARBShaderDrawParameters;
         unsigned _defaultAttribStorageVertexCapacity = 1000*1024; // 1M vertices (for just float coordinates ~12MiB, including normals, color and texCoord, ~36MiB)
         unsigned _defaultAttribStorageIndexCapacity = 4000*1024; // 4M indices (~16MiB)

         unsigned _bufferPosition;
         ProgressStamp _progressStamp; ///< Monotonically increasing number wrapping on overflow.

         std::shared_ptr<ge::gl::Program> _processDrawCommandsProgram;
         std::shared_ptr<ge::gl::Program> _ambientProgram;
         std::shared_ptr<ge::gl::Program> _phongProgram;
         std::shared_ptr<ge::gl::Program> _ambientUniformColorProgram;
         std::shared_ptr<ge::gl::Program> _phongUniformColorProgram;

         static void* mapBuffer(ge::gl::Buffer *buffer,
                                MappedBufferAccess requestedAccess,
                                void* &_mappedBufferPtr,
                                MappedBufferAccess &grantedAccess);
         static void unmapBuffer(ge::gl::Buffer *buffer,
                                 void* &mappedBufferPtr,
                                 MappedBufferAccess &currentAccess);

      public:

         static const float identityMatrix[16];
         inline const std::shared_ptr<MatrixList>& emptyMatrixList() const;

         RenderingContext();
         virtual ~RenderingContext();

         AttribConfig getAttribConfig(const AttribConfig::Configuration& config);
         inline AttribConfig getAttribConfig(const std::vector<AttribType>& attribTypes,bool ebo);
         inline AttribConfig getAttribConfig(const std::vector<AttribType>& attribTypes,bool ebo,
                                             AttribConfigId id);

         inline const AttribConfigInstances& attribConfigInstances();
         AttribConfig::Instance* getAttribConfigInstance(const AttribConfig::Configuration& config);
         void removeAttribConfigInstance(AttribConfigInstances::iterator it);

         inline bool getUseARBShaderDrawParameters() const;
         void setUseARBShaderDrawParameters(bool value);
         inline unsigned numAttribStorages() const;
         void onAttribStorageInit(AttribStorage *a);
         void onAttribStorageRelease(AttribStorage *a);
         inline unsigned defaultAttribStorageVertexCapacity() const;
         inline unsigned defaultAttribStorageIndexCapacity() const;
         inline void setDefaultAttribStorageVertexCapacity(unsigned capacity);
         inline void setDefaultAttribStorageIndexCapacity(unsigned capacity);

         inline PrimitiveStorage* primitiveStorage() const;                   ///< Returns BufferStorage that contains primitive set data of this graphics context. Any modification to the buffer must be done carefully to not break internal data consistency.
         inline DrawCommandStorage* drawCommandStorage() const;               ///< Returns BufferStorage that contains draw commands. Any modification to the buffer must be done carefully to not break internal data consistency.
         inline MatrixStorage* matrixStorage() const;
         inline ListControlStorage* matrixListControlStorage() const;
         inline StateSetStorage* stateSetStorage() const;                     ///< Returns BufferStorage that contains StateSet specific data.

         inline const std::shared_ptr<ge::gl::Buffer>& drawIndirectBuffer();  ///< Returns draw indirect buffer used for indirect rendering.
         inline float* cpuTransformationBuffer();

         void unmapBuffers();

         inline unsigned* transformationAllocation(unsigned id) const;
         inline ItemAllocationManager& transformationAllocationManager();
         inline const ItemAllocationManager& transformationAllocationManager() const;
         void setCpuTransformationBufferCapacity(unsigned numMatrices);

         virtual bool allocMeshData(Mesh &mesh,const AttribConfig& attribConfig,
                                    size_t numVertices,size_t numIndices,size_t numPrimitives);
         virtual bool reallocMeshData(Mesh &mesh,size_t numVertices,size_t numIndices,
                                      size_t numPrimitives,bool preserveContent=true);
         virtual  void freeMeshData(Mesh &mesh);

         virtual void uploadPrimitives(Mesh &mesh,const PrimitiveGpuData *bufferData,
                                       unsigned numPrimitives,unsigned dstIndex=0);
         virtual void setPrimitives(Mesh &mesh,const Primitive *primitiveList,
                                    unsigned numPrimitives,unsigned startIndex=0,
                                    bool truncate=true);
         virtual void setAndUploadPrimitives(Mesh &mesh,PrimitiveGpuData *nonConstBufferData,
                                             const Primitive *primitiveList,unsigned numPrimitives);
         inline void setAndUploadPrimitives(Mesh &mesh,PrimitiveGpuData *nonConstBufferData,
                                            const unsigned *modesAndOffsets4,unsigned numPrimitives);
         static void updateVertexOffsets(Mesh &mesh,void *primitiveBuffer,
                                         const Primitive *primitiveList,unsigned numPrimitives);
         static std::vector<Primitive> generatePrimitiveList(const unsigned *modesAndOffsets4,
                                                             unsigned numPrimitives);

         inline  void clearPrimitives(Mesh &mesh);
         virtual void setNumPrimitives(Mesh &mesh,unsigned num);

         inline DrawableId createDrawable(Mesh &mesh,
                                          MatrixList *matrixList,StateSet *stateSet);
         virtual DrawableId createDrawable(Mesh &mesh,
                                           const unsigned *primitiveIndices,
                                           const unsigned primtiveCount,
                                           MatrixList *matrixList,StateSet *stateSet);
         virtual void deleteDrawable(Mesh &mesh,DrawableId id);

         inline TransformationGraphList& transformationGraphs();
         inline const TransformationGraphList& transformationGraphs() const;
         virtual void addTransformationGraph(std::shared_ptr<Transformation>& transformation);
         virtual void removeTransformationGraph(std::shared_ptr<Transformation>& transformation);

         virtual void cancelAllAllocations();
         virtual void handleContextLost();

         inline unsigned bufferPosition() const;
         inline void setBufferPosition(unsigned pos);

         inline ProgressStamp progressStamp() const;
         inline void incrementProgressStamp();

         virtual void evaluateTransformationGraph();
         virtual void setupRendering();
         virtual void processDrawCommands();
         virtual void fenceSyncGpuComputation();
         virtual void render();
         virtual void frame();

         inline std::shared_ptr<StateSet> getOrCreateStateSet(const StateSetManager::GLState* state);
         inline std::shared_ptr<StateSet> findStateSet(const StateSetManager::GLState* state);
         inline StateSetManager::GLState* createGLState();
         inline const std::shared_ptr<StateSetManager>& stateSetManager();
         void setStateSetManager(const std::shared_ptr<StateSetManager>& stateSetManager);

         const std::shared_ptr<ge::gl::Program>& getProcessDrawCommandsProgram() const;
         const std::shared_ptr<ge::gl::Program>& getAmbientProgram() const;
         const std::shared_ptr<ge::gl::Program>& getPhongProgram() const;
         const std::shared_ptr<ge::gl::Program>& getAmbientUniformColorProgram() const;
         const std::shared_ptr<ge::gl::Program>& getPhongUniformColorProgram() const;
         enum class ProgramType { AMBIENT_PASS,LIGHT_PASS,AMBIENT_AND_LIGHT_PASS };
         const std::shared_ptr<ge::gl::Program>& getProgram(ProgramType type,bool uniformColor) const;

         std::shared_ptr<ge::gl::Texture> cachedTexture(const std::string& path) const;
         inline void addCacheTexture(const std::string &path,const std::shared_ptr<ge::gl::Texture>& texture);

         static inline const std::shared_ptr<RenderingContext>& current();
         static void setCurrent(const std::shared_ptr<RenderingContext>& ptr);

         static void global_init();
         static void global_finalize();

      protected:

         struct AutoInitRenderingContext {
            bool initialized; // initialized to false if struct is declared static
            bool usingNiftyCounter;
            typedef std::shared_ptr<RenderingContext> Ptr;
            std::aligned_storage<sizeof(Ptr),std::alignment_of<Ptr>::value>::type ptr;
            inline std::shared_ptr<RenderingContext>& get()  { return reinterpret_cast<Ptr&>(ptr); }
            AutoInitRenderingContext();
            ~AutoInitRenderingContext();
         };
         struct NoExport { // workaround for MSVC 2015: thread_local variables can not have DLL-export interface,
                           // solution: nested structures are not DLL-exported and do not inherit DLL-export of parent class
            static thread_local AutoInitRenderingContext _currentContext;
         };

         struct ProgramConfig {
            ProgramType type;
            bool uniformColor;
            bool operator<(const ProgramConfig& rhs) const;
         };
         mutable std::map<ProgramConfig,std::shared_ptr<ge::gl::Program>> _programCache;
         static std::shared_ptr<ge::gl::Program> createProgram(ProgramType type,bool uniformColor,
                                                               bool useARBShaderDrawParameters=false);

      };


      static ge::core::InitAndFinalize renderingContextInitAndFinalize(
            &RenderingContext::global_init,&RenderingContext::global_finalize);


      inline RenderingContext::MappedBufferAccess& operator|=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b);
      inline RenderingContext::MappedBufferAccess& operator&=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b);
      inline RenderingContext::MappedBufferAccess& operator+=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b);
      inline RenderingContext::MappedBufferAccess& operator-=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b);
   }
}
#endif
