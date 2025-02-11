/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/debug_settings/debug_settings_manager.h"
#include "shared/source/helpers/api_specific_config.h"

#include "opencl/source/os_interface/ocl_reg_path.h"

namespace NEO {
bool ApiSpecificConfig::isStatelessCompressionSupported() {
    return true;
}

bool ApiSpecificConfig::isBcsSplitWaSupported() {
    return true;
}

bool ApiSpecificConfig::getGlobalBindlessHeapConfiguration() {
    return false;
}

bool ApiSpecificConfig::getBindlessMode() {
    if (DebugManager.flags.UseBindlessMode.get() != -1) {
        return DebugManager.flags.UseBindlessMode.get();
    } else {
        return false;
    }
}

bool ApiSpecificConfig::isDeviceAllocationCacheEnabled() {
    return false;
}

bool ApiSpecificConfig::isDynamicPostSyncAllocLayoutEnabled() {
    return false;
}

bool ApiSpecificConfig::isRelaxedOrderingEnabled() {
    return true;
}

ApiSpecificConfig::ApiType ApiSpecificConfig::getApiType() {
    return ApiSpecificConfig::OCL;
}

std::string ApiSpecificConfig::getName() {
    return "ocl";
}

uint64_t ApiSpecificConfig::getReducedMaxAllocSize(uint64_t maxAllocSize) {
    return maxAllocSize / 2;
}

const char *ApiSpecificConfig::getRegistryPath() {
    return oclRegPath;
}
} // namespace NEO
