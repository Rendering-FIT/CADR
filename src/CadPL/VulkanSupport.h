// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <CadR/VulkanSupport.h>

namespace CadPL {


CADPL_EXPORT bool areRequiredFeaturesSupported(const CadR::DeviceFeatures& features);
CADPL_EXPORT std::string missingRequiredFeatures(const CadR::DeviceFeatures& features);
CADPL_EXPORT void setRequiredFeatures(CadR::DeviceFeatures& features);
CADPL_EXPORT void setRequiredAndOptionalFeatures(const CadR::DeviceFeatures& supportedFeatures,
	CadR::DeviceFeatures& featuresUsed);


}
