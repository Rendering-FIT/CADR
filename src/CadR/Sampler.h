#ifndef CADR_SAMPLER_HEADER
# define CADR_SAMPLER_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  undef CADR_NO_INLINE_FUNCTIONS
# else
# endif
# include <vulkan/vulkan.hpp>

namespace CadR {

class Renderer;


class CADR_EXPORT Sampler {
protected:
	vk::Sampler _sampler;
	Renderer* _renderer;
public:

	// construction and destruction
	inline Sampler(CadR::Renderer& renderer) noexcept;
	inline Sampler(CadR::Renderer& renderer, const vk::SamplerCreateInfo& samplerCreateInfo);
	inline ~Sampler() noexcept;

	// move and copy
	inline Sampler(Sampler&& other) noexcept;
	Sampler(const Sampler&) = delete;
	inline Sampler& operator=(Sampler&& rhs) noexcept;
	Sampler& operator=(const Sampler&) = delete;

	// functions
	inline void create(const vk::SamplerCreateInfo& samplerCreateInfo);
	inline void destroy() noexcept;
	inline vk::Sampler handle() const;
};


}

#endif


// inline methods
#if !defined(CADR_SAMPLER_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_SAMPLER_INLINE_FUNCTIONS
# include <CadR/Renderer.h>
# include <CadR/VulkanDevice.h>
namespace CadR {

inline Sampler::Sampler(CadR::Renderer& renderer) noexcept  : _renderer(&renderer) {}
inline Sampler::Sampler(CadR::Renderer& renderer, const vk::SamplerCreateInfo& samplerCreateInfo)  : _sampler(renderer.device().createSampler(samplerCreateInfo)), _renderer(&renderer) {}
inline Sampler::Sampler(Sampler&& other) noexcept  : _sampler(other._sampler), _renderer(other._renderer) { other._sampler=nullptr; }
inline Sampler::~Sampler() noexcept  { destroy(); }
inline Sampler& Sampler::operator=(Sampler&& rhs) noexcept  { destroy(); _sampler=rhs._sampler; _sampler=nullptr; _renderer=rhs._renderer; return *this; }
inline void Sampler::create(const vk::SamplerCreateInfo& samplerCreateInfo)  { destroy(); _sampler=_renderer->device().createSampler(samplerCreateInfo); }
inline void Sampler::destroy() noexcept  { if(_sampler) { _renderer->device().destroy(_sampler); _sampler=nullptr; } }
inline vk::Sampler Sampler::handle() const  { return _sampler; }

}

#endif
