#include <CadR/VulkanSupport.h>
#include <cstring>

using namespace std;
using namespace CadR;


void CadR::DeviceFeatures::clear()
{
	memset(reinterpret_cast<char*>(&vulkan10)+sizeof(vk::BaseOutStructure), 0, sizeof(vulkan10)-sizeof(vk::BaseOutStructure));
	memset(reinterpret_cast<char*>(&vulkan11)+sizeof(vk::BaseOutStructure), 0, sizeof(vulkan11)-sizeof(vk::BaseOutStructure));
	memset(reinterpret_cast<char*>(&vulkan12)+sizeof(vk::BaseOutStructure), 0, sizeof(vulkan12)-sizeof(vk::BaseOutStructure));
	memset(reinterpret_cast<char*>(&vulkan13)+sizeof(vk::BaseOutStructure), 0, sizeof(vulkan13)-sizeof(vk::BaseOutStructure));
	memset(reinterpret_cast<char*>(&vulkan14)+sizeof(vk::BaseOutStructure), 0, sizeof(vulkan14)-sizeof(vk::BaseOutStructure));
}


bool CadR::areRequiredFeaturesSupported(const DeviceFeatures& features)
{
	return
		features.vulkan10.features.multiDrawIndirect &&  // required for StateSet rendering
		features.vulkan10.features.shaderInt64 &&  // required for pointer arithmetics and data access
		features.vulkan11.shaderDrawParameters &&  // required for gl_DrawID variable used in vertex shaders
		features.vulkan12.bufferDeviceAddress;  // required for pointer-based data access
}


string CadR::missingRequiredFeatures(const DeviceFeatures& features)
{
	string r;
	if(!features.vulkan10.features.multiDrawIndirect)
		r += ", multiDrawIndirect";
	if(!features.vulkan10.features.shaderInt64)
		r += ", shaderInt64";
	if(!features.vulkan11.shaderDrawParameters)
		r += ", shaderDrawParameters";
	if(!features.vulkan12.bufferDeviceAddress)
		r += ", bufferDeviceAddress";
	return r;
}


void CadR::setRequiredFeatures(DeviceFeatures& features)
{
	features.vulkan10.features.multiDrawIndirect = true;
	features.vulkan10.features.shaderInt64 = true;
	features.vulkan11.shaderDrawParameters = true;
	features.vulkan12.bufferDeviceAddress = true;
}


void CadR::setRequiredAndOptionalFeatures(
	const DeviceFeatures& supportedFeatures, DeviceFeatures& featuresUsed)
{
	featuresUsed.vulkan10.features.multiDrawIndirect = true;
	featuresUsed.vulkan10.features.shaderInt64 = true;
	featuresUsed.vulkan11.shaderDrawParameters = true;
	featuresUsed.vulkan12.bufferDeviceAddress = true;
}
