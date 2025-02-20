#ifndef CADR_TEXTURE_HEADER
# define CADR_TEXTURE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/ImageAllocation.h>
#  include <CadR/StateSet.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/ImageAllocation.h>
#  include <CadR/StateSet.h>
# endif
# include <vulkan/vulkan.hpp>
# include <boost/intrusive/list.hpp>
# include <memory>

namespace CadR {

class StateSet;
class VulkanDevice;



class CADR_EXPORT Texture {
protected:
	vk::ImageView _imageView;
	CadR::VulkanDevice* _device;
	vk::Sampler _sampler;
	vk::ImageViewCreateInfo _imageViewCreateInfo;

	ImageChangedCallback _imageChangedCallback;
	boost::intrusive::list<
		StateSetDescriptorUpdater,
		boost::intrusive::member_hook<
			StateSetDescriptorUpdater,
			boost::intrusive::list_member_hook<
				boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
			&StateSetDescriptorUpdater::_updaterHook>,
		boost::intrusive::constant_time_size<false>
	> _descriptorUpdaterList;

	void recreateHandles(vk::Image image);
	void releaseHandles() noexcept;
	void initImageChangedCallback();
	void callDescriptorUpdaters();
public:

	// construction and destruction
	Texture(ImageAllocation& a, const vk::ImageViewCreateInfo& imageViewCreateInfo,
			const vk::Sampler& sampler, CadR::VulkanDevice& device);
	Texture(ImageAllocation& a, const vk::ImageViewCreateInfo& imageViewCreateInfo,
			std::unique_ptr<void>&& imageViewCreateInfoPNext, const vk::Sampler& sampler,
			CadR::VulkanDevice& device);
	~Texture() noexcept;

	// move and copy
	Texture(Texture&& other) noexcept;
	Texture(const Texture&) = delete;
	Texture& operator=(Texture&& rhs) noexcept;
	Texture& operator=(const Texture&) = delete;

	// getters
	inline vk::ImageView imageView() const;
	inline VulkanDevice& device() const;
	inline vk::Sampler sampler() const;

	// functions
	void attachStateSet(StateSet& ss,
	                    vk::DescriptorSet descriptorSet,
	                    void(*descriptorUpdateFunc)(StateSet& ss, vk::DescriptorSet descriptorSet, Texture& t));

};



// inline functions
inline vk::ImageView Texture::imageView() const  { return _imageView; }
inline VulkanDevice& Texture::device() const  { return *_device; }
inline vk::Sampler Texture::sampler() const  { return _sampler; }
inline void Texture::callDescriptorUpdaters()  { for(auto& u : _descriptorUpdaterList) u.updateFunc(*this); }

}

#endif
