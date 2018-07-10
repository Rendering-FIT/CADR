#pragma once

#include <memory>
#include <vector>
#include <CADR/Export.h>

namespace cd {

class AttribConfig;
struct Buffer;
class Scene;


namespace Topology {
	static constexpr const uint8_t PointList = 0;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_POINT_LIST=0 and GL_POINTS=0.
	static constexpr const uint8_t LineList = 1;   // Corresponds with VK_PRIMITIVE_TOPOLOGY_LINE_LIST=1 and GL_LINES=1.
	static constexpr const uint8_t LineStrip = 2;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_LINE_STRIP=2 and GL_LINE_STRIP=3.
	static constexpr const uint8_t TriangleList = 3;   // Corresponds with VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST=3 and GL_TRIANGLES=4.
	static constexpr const uint8_t TriangleStrip = 4;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP=4 and GL_TRIANGLE_STRIP=5.
	static constexpr const uint8_t TriangleFan = 5;    // Corresponds with VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN=5 and GL_TRIANGLE_FAN=6.
	static constexpr const uint8_t LineListWithAdjacency = 6;   // Corresponds with VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY=6 and GL_LINES_ADJACENCY=0xA.
	static constexpr const uint8_t LineStripWithAdjacency = 7;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY=7 and GL_LINE_STRIP_ADJACENCY=0xB.
	static constexpr const uint8_t TriangleListWithAdjacency = 8;   // Corresponds with VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY=8 and GL_TRIANGLES_ADJACENCY=0xC.
	static constexpr const uint8_t TriangleStripWithAdjacency = 9;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY=9 and GL_TRIANGLE_STRIP_ADJACENCY=0xD.
	static constexpr const uint8_t PatchList = 10;  // Corresponds with VK_PRIMITIVE_TOPOLOGY_PATCH_LIST=10 and GL_PATCHES=0xE.
	static constexpr const uint8_t RangeSize = PatchList+1;  // Number of topologies
	static constexpr const uint8_t Invalid = 0xff;  // Invalid topology. If used inside Primitive, the primitive will be ignored during rendering.
};


/** DrawCommand represents single rendering command (such as vkCmdDraw, glDrawElements,...).
 *  In reality, it is used in indirect manner. So, it is similar to VkDrawIndirectCommand
 *  and VkDrawIndexedIndirect structures.
 *
 *  The size of DrawCommand is strictly 12 bytes (verified by assert in the source code).
 */
struct DrawCommand {
	uint8_t topology;
	bool indexed : 8;
	unsigned first;
	unsigned count;
	inline DrawCommand() = default;
	constexpr inline DrawCommand(uint8_t topology,bool indexed,unsigned first,unsigned count);
};


/** Drawable class represents geometry data that can be rendered.
 *  It is composed of vertex data (vertex attributes), indices (optionally) and draw commands.
 *
 *  The class is meant to handle hundreds of thousands rendered drawables in real-time.
 *  This is useful for many CAD applications and scenes composed of many objects.
 *
 *  Geode class is used to render Drawables using particular StateSet and instancing them by
 *  evaluating Transformation graph.
 */
class CADR_EXPORT Drawable {
protected:

	AttribStorage* _attribStorage;  ///< AttribStorage where vertex and index data are stored.
	size_t _verticesDataId;  ///< Id of vertex data allocation inside AttribStorage.
	size_t _indicesDataId;  ///< Id of index data allocation inside AttribStorage.
	size_t _drawCommandDataId;  ///< Id od DrawCommand data allocation.
	DrawCommandList _drawCommandList;
	GeodeList _geodeList;

public:

	static inline Drawable* create(Scene* scene);
	static inline std::shared_ptr<Drawable> make_shared(Scene* scene);
	virtual ~Drawable() {}

	virtual void allocData(const AttribConfig& attribConfig,size_t numVertices,
	                       size_t numIndices,size_t numDrawCommands) = 0;
	virtual void reallocData(size_t numVertices,size_t numIndices,
	                         size_t numDrawCommands,bool preserveContent=true) = 0;
	virtual void freeData() = 0;

	virtual void uploadVertices(std::vector<Buffer>&& vertexData,size_t dstIndex=0) = 0;
	virtual void uploadAttrib(unsigned attribIndex,Buffer&& attribData,size_t dstIndex=0) = 0;
	virtual void uploadIndices(std::vector<uint32_t>&& indexData,size_t dstIndex=0) = 0;
	virtual void uploadDrawCommands(const std::vector<DrawCommand>&& drawCommands,
	                                size_t dstIndex=0) = 0;

};


}


// inline methods
#include <CADR/Factory.h>
namespace ri {

constexpr inline DrawCommand::DrawCommand(uint8_t topology,bool indexed,unsigned first,unsigned count) : topology(topology), indexed(indexed), first(first), count(count)  {}
inline Drawable* Drawable::create(Scene* scene)  { return Factory::get()->createDrawable(scene); }
inline std::shared_ptr<Drawable> Drawable::make_shared(Scene* scene)  { return Factory::get()->makeDrawable(scene); }

}
