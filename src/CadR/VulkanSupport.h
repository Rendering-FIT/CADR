// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

namespace CadR {


struct DeviceFeatures {
	vk::PhysicalDeviceFeatures2 vulkan10;
	vk::PhysicalDeviceVulkan11Features vulkan11;
	vk::PhysicalDeviceVulkan12Features vulkan12;
	vk::PhysicalDeviceVulkan13Features vulkan13;
	vk::PhysicalDeviceVulkan14Features vulkan14;

	DeviceFeatures() = default;
	inline DeviceFeatures(uint32_t vulkanVersion);
	void clear();
	inline void link(uint32_t vulkanVersion);
};

CADR_EXPORT bool areRequiredFeaturesSupported(const DeviceFeatures& features);
CADR_EXPORT std::string missingRequiredFeatures(const DeviceFeatures& features);
CADR_EXPORT void setRequiredFeatures(DeviceFeatures& features);
CADR_EXPORT void setRequiredAndOptionalFeatures(const DeviceFeatures& supportedFeatures,
	DeviceFeatures& featuresUsed);


// inline functions
inline DeviceFeatures::DeviceFeatures(uint32_t vulkanVersion)  { link(vulkanVersion); }
inline void DeviceFeatures::link(uint32_t vulkanVersion)  { vulkan10.pNext=vulkanVersion>=VK_API_VERSION_1_1?&vulkan11:nullptr; vulkan11.pNext=vulkanVersion>=VK_API_VERSION_1_2?&vulkan12:nullptr; vulkan12.pNext=vulkanVersion>=VK_API_VERSION_1_3?&vulkan13:nullptr; vulkan13.pNext=vulkanVersion>=VK_API_VERSION_1_4?&vulkan14:nullptr; vulkan14.pNext=nullptr; }

}
