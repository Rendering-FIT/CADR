#include <CadR/Texture.h>
#include <CadR/Exceptions.h>
#include <CadR/ImageAllocation.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;



Texture::~Texture() noexcept
{
	_descriptorUpdaterList.clear_and_dispose([](StateSetDescriptorUpdater* u){ delete u; });
	releaseHandles();
	delete _imageViewCreateInfo.pNext;
}


Texture::Texture(ImageAllocation& a, const vk::ImageViewCreateInfo& imageViewCreateInfo,
		const vk::Sampler& sampler, CadR::VulkanDevice& device)
	: _device(&device)
	, _imageViewCreateInfo(imageViewCreateInfo)
	, _sampler(sampler)
{
	if(imageViewCreateInfo.pNext)
		throw LogicError("Texture::Texture(): imageViewCreateInfo.pNext must be null. Please, use different constructor.");
	_imageViewCreateInfo.image = a.image();
	_imageView = _device->createImageView(_imageViewCreateInfo);
	initImageChangedCallback();
	callDescriptorUpdaters();
}


Texture::Texture(ImageAllocation& a, const vk::ImageViewCreateInfo& imageViewCreateInfo,
                 unique_ptr<void>&& imageViewCreateInfoPNext, const vk::Sampler& sampler,
                 CadR::VulkanDevice& device)
	: _device(&device)
	, _imageViewCreateInfo(imageViewCreateInfo)
	, _sampler(sampler)
{
	_imageViewCreateInfo.pNext = imageViewCreateInfoPNext.release();
	_imageViewCreateInfo.image = a.image();
	_imageView = _device->createImageView(_imageViewCreateInfo);
	initImageChangedCallback();
	callDescriptorUpdaters();
}


Texture::Texture(Texture&& other) noexcept
	: _imageView(other._imageView)
	, _device(other._device)
	, _sampler(other._sampler)
	, _imageViewCreateInfo(_imageViewCreateInfo)
{
	other._imageView = nullptr;
	other._imageViewCreateInfo.pNext = nullptr;
	initImageChangedCallback();
	other._imageChangedCallback._callbackHook.unlink();
}


Texture& Texture::operator=(Texture&& rhs) noexcept
{
	releaseHandles();
	_imageView = rhs._imageView;
	_device = rhs._device;
	_sampler = rhs._sampler;
	_imageViewCreateInfo = rhs._imageViewCreateInfo;
	rhs._imageView = nullptr;
	rhs._imageViewCreateInfo.pNext = nullptr;
	initImageChangedCallback();
	rhs._imageChangedCallback._callbackHook.unlink();
	return *this;
}


void Texture::releaseHandles() noexcept
{
	if(_imageView) {
		_device->destroy(_imageView);
		_imageView = nullptr;
	}
}


void Texture::recreateHandles(vk::Image image)
{
	releaseHandles();

	// imageView
	_imageViewCreateInfo.image = image;
	_imageView = _device->createImageView(_imageViewCreateInfo);
}


void Texture::initImageChangedCallback()
{
	_imageChangedCallback.imageChanged =
		bind(
			[](vk::Image image, Texture* t) {
				t->recreateHandles(image);
				t->callDescriptorUpdaters();
			},
			placeholders::_1,
			this
		);
}


void Texture::attachStateSet(StateSet& ss,
	vk::DescriptorSet descriptorSet,
	void(*descriptorUpdateFunc)(StateSet& ss, vk::DescriptorSet descriptorSet, Texture& t))
{
	StateSetDescriptorUpdater& u =
		ss.createDescriptorUpdater(
			bind(descriptorUpdateFunc, ref(ss), descriptorSet, placeholders::_1),
			descriptorSet
		);
	_descriptorUpdaterList.push_back(u);
	u.updateFunc(*this);
}
