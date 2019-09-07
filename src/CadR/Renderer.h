#pragma once

#include <list>
#include <map>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <CadR/AllocationManagers.h>
#include <CadR/Export.h>

namespace CadR {

class AttribConfig;
class AttribStorage;
class Mesh;
class StagingBuffer;
class VulkanDevice;
class VulkanInstance;


class Renderer final {
private:

	VulkanDevice* _device;
	uint32_t _graphicsQueueFamily;
	vk::Queue _graphicsQueue;

	std::map<AttribConfig,std::list<AttribStorage>> _attribStorages;
	AttribStorage* _emptyStorage;
	vk::Buffer _indexBuffer;
	vk::Buffer _primitiveSetBuffer;
	ArrayAllocationManager<Mesh> _indexAllocationManager;  ///< Allocation manager for index data.
	ItemAllocationManager _primitiveSetAllocationManager;  ///< Allocation manager for primitiveSet data.

	vk::PhysicalDeviceMemoryProperties _memoryProperties;
	vk::CommandPool _commandPoolTransient;
	vk::CommandBuffer _uploadCommandBuffer;
	std::list<std::tuple<vk::Buffer,vk::DeviceMemory>> _objectsToDeleteAfterCopyOperation;

	static Renderer* _instance;

public:

	CADR_EXPORT static Renderer* get();
	CADR_EXPORT static void set(Renderer* r);

	CADR_EXPORT Renderer() = delete;
	CADR_EXPORT Renderer(VulkanDevice* device,VulkanInstance* instance,vk::PhysicalDevice physicalDevice,
	                     uint32_t graphicsQueueFamily);
	CADR_EXPORT ~Renderer();

	CADR_EXPORT VulkanDevice* device() const;
	CADR_EXPORT vk::Buffer indexBuffer() const;

	CADR_EXPORT AttribStorage* getOrCreateAttribStorage(const AttribConfig& ac);
	CADR_EXPORT std::map<AttribConfig,std::list<AttribStorage>>& getAttribStorages();
	CADR_EXPORT const AttribStorage* emptyStorage() const;
	CADR_EXPORT AttribStorage* emptyStorage();

	CADR_EXPORT vk::DeviceMemory allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags);
	CADR_EXPORT vk::CommandBuffer uploadCommandBuffer() const;
	CADR_EXPORT void scheduleCopyOperation(StagingBuffer& stagingBuffer);
	CADR_EXPORT void executeCopyOperations();

	CADR_EXPORT void uploadIndices(Mesh& m,std::vector<uint32_t>&& indexData,size_t dstIndex=0);

	CADR_EXPORT const ArrayAllocation<Mesh>& indexAllocation(unsigned id) const;  ///< Returns index allocation for particular id.
	CADR_EXPORT ArrayAllocation<Mesh>& indexAllocation(unsigned id);   ///< Returns index allocation for particular id. Modify the returned data only with caution.
	CADR_EXPORT const ArrayAllocationManager<Mesh>& indexAllocationManager() const;
	CADR_EXPORT ArrayAllocationManager<Mesh>& indexAllocationManager();

	CADR_EXPORT const ItemAllocationManager& primitiveSetAllocationManager() const;
	CADR_EXPORT ItemAllocationManager& primitiveSetAllocationManager();

protected:
	CADR_EXPORT void purgeObjectsToDeleteAfterCopyOperation();
};


// inline methods
inline Renderer* Renderer::get()  { return _instance; }
inline void Renderer::set(Renderer* r)  { _instance=r; }
inline VulkanDevice* Renderer::device() const  { return _device; }
inline vk::Buffer Renderer::indexBuffer() const  { return _indexBuffer; }
inline std::map<AttribConfig,std::list<AttribStorage>>& Renderer::getAttribStorages()  { return _attribStorages; }
inline const AttribStorage* Renderer::emptyStorage() const  { return _emptyStorage; }
inline AttribStorage* Renderer::emptyStorage()  { return _emptyStorage; }
inline const ArrayAllocation<Mesh>& Renderer::indexAllocation(unsigned id) const  { return _indexAllocationManager[id]; }
inline ArrayAllocation<Mesh>& Renderer::indexAllocation(unsigned id)  { return _indexAllocationManager[id]; }
inline const ArrayAllocationManager<Mesh>& Renderer::indexAllocationManager() const  { return _indexAllocationManager; }
inline ArrayAllocationManager<Mesh>& Renderer::indexAllocationManager()  { return _indexAllocationManager; }
inline const ItemAllocationManager& Renderer::primitiveSetAllocationManager() const  { return _primitiveSetAllocationManager; }
inline ItemAllocationManager& Renderer::primitiveSetAllocationManager()  { return _primitiveSetAllocationManager; }
inline vk::CommandBuffer Renderer::uploadCommandBuffer() const  { return _uploadCommandBuffer; }


}

#if 0
// some includes need to be placed before GE_RG_RENDERING_CONTEXT_H define
// to prevent problems of circular includes
#include <geRG/AttribConfig.h>
#include <geRG/MatrixList.h>
#include <geRG/Mesh.h>

#ifndef GE_RG_RENDERING_CONTEXT_H
#define GE_RG_RENDERING_CONTEXT_H

#include <memory>
#include <geRG/Export.h>
#include <geRG/AllocationManagers.h>
#include <geRG/Basics.h>
#include <geRG/BufferStorage.h>
#include <geRG/Drawable.h>
#include <geRG/DrawCommand.h>
#include <geRG/Primitive.h>
#include <geRG/ProgressStamp.h>
#include <geRG/StateSet.h>
#include <geRG/StateSetManager.h>
#include <geGL/OpenGLContext.h>
#include <geCore/InitAndFinalize.h>

namespace ge
{
   namespace gl
   {
      class Buffer;
      class Program;
      class Texture;
   }
   namespace rg
   {
      class Transformation;


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



// inline methods

namespace ge
{
   namespace rg
   {
      inline const std::shared_ptr<MatrixList>& RenderingContext::emptyMatrixList() const
      { if(_emptyMatrixList==nullptr) const_cast<RenderingContext*>(this)->_emptyMatrixList=std::make_shared<MatrixList>(0,0,0); return _emptyMatrixList; }
      inline const RenderingContext::AttribConfigInstances& RenderingContext::attribConfigInstances()  { return _attribConfigInstances; }
      inline AttribConfig RenderingContext::getAttribConfig(const std::vector<AttribType>& attribTypes,bool ebo)
      { return getAttribConfig(attribTypes,ebo,AttribConfig::getId(attribTypes,ebo)); }
      inline AttribConfig RenderingContext::getAttribConfig(const std::vector<AttribType>& attribTypes,bool ebo,AttribConfigId id)
      { return getAttribConfig(AttribConfig::Configuration(attribTypes,ebo,id)); }
      inline bool RenderingContext::getUseARBShaderDrawParameters() const  { return _useARBShaderDrawParameters; }
      inline unsigned RenderingContext::numAttribStorages() const  { return _numAttribStorages; }
      inline unsigned RenderingContext::defaultAttribStorageVertexCapacity() const  { return _defaultAttribStorageVertexCapacity; }
      inline unsigned RenderingContext::defaultAttribStorageIndexCapacity() const  { return _defaultAttribStorageIndexCapacity; }
      inline void RenderingContext::setDefaultAttribStorageVertexCapacity(unsigned capacity)  { _defaultAttribStorageVertexCapacity=capacity; }
      inline void RenderingContext::setDefaultAttribStorageIndexCapacity(unsigned capacity)  { _defaultAttribStorageIndexCapacity=capacity; }
      inline PrimitiveStorage* RenderingContext::primitiveStorage() const  { return &_primitiveStorage; }
      inline DrawCommandStorage* RenderingContext::drawCommandStorage() const  { return &_drawCommandStorage; }
      inline MatrixStorage* RenderingContext::matrixStorage() const  { return &_matrixStorage; }
      inline ListControlStorage* RenderingContext::matrixListControlStorage() const  { return &_matrixListControlStorage; }
      inline StateSetStorage* RenderingContext::stateSetStorage() const  { return &_stateSetStorage; }
      inline const std::shared_ptr<ge::gl::Buffer>& RenderingContext::drawIndirectBuffer()  { return _drawIndirectBuffer; }
      inline float* RenderingContext::cpuTransformationBuffer()  { return _cpuTransformationBuffer; }
      inline unsigned* RenderingContext::transformationAllocation(unsigned id) const  { return _transformationAllocationManager[id]; }
      inline ItemAllocationManager& RenderingContext::transformationAllocationManager()  { return _transformationAllocationManager; }
      inline const ItemAllocationManager& RenderingContext::transformationAllocationManager() const  { return _transformationAllocationManager; }
      inline void RenderingContext::freeVertexData(Mesh &mesh)  { if(mesh.attribStorage()) mesh.attribStorage()->freeData(mesh); }
      inline void RenderingContext::setAndUploadPrimitives(ge::rg::Mesh& mesh,ge::rg::PrimitiveGpuData* nonConstBufferData,const unsigned* modesAndOffsets4,unsigned numPrimitives)
      { setAndUploadPrimitives(mesh,nonConstBufferData,generatePrimitiveList(modesAndOffsets4,numPrimitives).data(),numPrimitives); }
      inline void RenderingContext::clearPrimitives(Mesh &mesh)  { setNumPrimitives(mesh,0); }
      inline DrawableId RenderingContext::createDrawable(Mesh &mesh,MatrixList *matrixList,StateSet *stateSet)
      { return createDrawable(mesh,nullptr,0,matrixList,stateSet); }
      inline RenderingContext::TransformationGraphList& RenderingContext::transformationGraphs()  { return _transformationGraphs; }
      inline const RenderingContext::TransformationGraphList& RenderingContext::transformationGraphs() const  { return _transformationGraphs; }
      inline unsigned RenderingContext::bufferPosition() const  { return _bufferPosition; }
      inline void RenderingContext::setBufferPosition(unsigned pos)  { _bufferPosition=pos; }
      inline ProgressStamp RenderingContext::progressStamp() const  { return _progressStamp; }
      inline void RenderingContext::incrementProgressStamp()  { ++_progressStamp; }
      inline const std::shared_ptr<RenderingContext>& RenderingContext::current()
      { return NoExport::_currentContext.get(); }

      inline std::shared_ptr<StateSet> RenderingContext::getOrCreateStateSet(const StateSetManager::GLState* state)
      { return _stateSetManager->getOrCreateStateSet(state); }
      inline std::shared_ptr<StateSet> RenderingContext::findStateSet(const StateSetManager::GLState* state)
      { return _stateSetManager->findStateSet(state); }
      inline StateSetManager::GLState* RenderingContext::createGLState()  { return _stateSetManager->createGLState(); }
      inline const std::shared_ptr<StateSetManager>& RenderingContext::stateSetManager()  { return _stateSetManager; }

      inline void RenderingContext::addCacheTexture(const std::string &path,const std::shared_ptr<ge::gl::Texture>& texture)  { _textureCache[path]=texture; }

      inline RenderingContext::MappedBufferAccess& operator|=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b)
      { (uint8_t&)a|=(uint8_t)b; return a; }
      inline RenderingContext::MappedBufferAccess& operator&=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b)
      { (uint8_t&)a&=(uint8_t)b; return a; }
      inline RenderingContext::MappedBufferAccess& operator+=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b)
      { (uint8_t&)a|=(uint8_t)b; return a; }
      inline RenderingContext::MappedBufferAccess& operator-=(RenderingContext::MappedBufferAccess &a,RenderingContext::MappedBufferAccess b)
      { (uint8_t&)a&=(uint8_t)b; return a; }

   }
}

#endif // GE_RG_RENDERING_CONTEXT_H
#endif
