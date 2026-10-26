#include <CadPL/VulkanSupport.h>

using namespace std;
using namespace CadR;
using namespace CadPL;


bool CadPL::areRequiredFeaturesSupported(const DeviceFeatures& features)
{
	return
		features.vulkan10.features.geometryShader &&  // required for many non-fully optimized shaders
		features.vulkan12.runtimeDescriptorArray &&  // required for texturing
		features.vulkan12.descriptorBindingSampledImageUpdateAfterBind &&  // required for texturing
		features.vulkan12.descriptorBindingUpdateUnusedWhilePending &&  // required for texturing
		features.vulkan12.descriptorBindingPartiallyBound &&  // required for texturing
		features.vulkan12.descriptorBindingVariableDescriptorCount;  // required for texturing
}


string CadPL::missingRequiredFeatures(const DeviceFeatures& features)
{
	string r;
	if(!features.vulkan10.features.geometryShader)
		r += ", geometryShader";
	if(!features.vulkan12.runtimeDescriptorArray)
		r += ", runtimeDescriptorArray";
	if(!features.vulkan12.descriptorBindingSampledImageUpdateAfterBind)
		r += ", descriptorBindingSampledImageUpdateAfterBind";
	if(!features.vulkan12.descriptorBindingUpdateUnusedWhilePending)
		r += ", descriptorBindingUpdateUnusedWhilePending";
	if(!features.vulkan12.descriptorBindingPartiallyBound)
		r += ", descriptorBindingPartiallyBound";
	if(!features.vulkan12.descriptorBindingVariableDescriptorCount)
		r += ", descriptorBindingVariableDescriptorCount";
	return r;
}


void CadPL::setRequiredFeatures(DeviceFeatures& features)
{
	features.vulkan10.features.geometryShader = true;
	features.vulkan12.runtimeDescriptorArray = true;
	features.vulkan12.descriptorBindingSampledImageUpdateAfterBind = true;
	features.vulkan12.descriptorBindingUpdateUnusedWhilePending = true;
	features.vulkan12.descriptorBindingPartiallyBound = true;
	features.vulkan12.descriptorBindingVariableDescriptorCount = true;
}


void CadPL::setRequiredAndOptionalFeatures(
	const DeviceFeatures& supportedFeatures, DeviceFeatures& featuresUsed)
{
	featuresUsed.vulkan10.features.geometryShader = true;
	featuresUsed.vulkan12.runtimeDescriptorArray = true;
	featuresUsed.vulkan12.descriptorBindingSampledImageUpdateAfterBind = true;
	featuresUsed.vulkan12.descriptorBindingUpdateUnusedWhilePending = true;
	featuresUsed.vulkan12.descriptorBindingPartiallyBound = true;
	featuresUsed.vulkan12.descriptorBindingVariableDescriptorCount = true;
}
