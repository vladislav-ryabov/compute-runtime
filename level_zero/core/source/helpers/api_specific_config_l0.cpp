/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/debug_settings/debug_settings_manager.h"
#include "shared/source/helpers/api_specific_config.h"

#include "level_zero/core/source/compiler_interface/l0_reg_path.h"

namespace NEO {
bool ApiSpecificConfig::isStatelessCompressionSupported() {
    return false;
}

bool ApiSpecificConfig::isBcsSplitWaSupported() {
    return true;
}

bool ApiSpecificConfig::getGlobalBindlessHeapConfiguration() {
    return DebugManager.flags.UseExternalAllocatorForSshAndDsh.get();
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
    return (NEO::DebugManager.flags.EnableDynamicPostSyncAllocLayout.get() == 1);
}

bool ApiSpecificConfig::isRelaxedOrderingEnabled() {
    return false;
}

ApiSpecificConfig::ApiType ApiSpecificConfig::getApiType() {
    return ApiSpecificConfig::L0;
}

std::string ApiSpecificConfig::getName() {
    return "l0";
}

uint64_t ApiSpecificConfig::getReducedMaxAllocSize(uint64_t maxAllocSize) {
    return maxAllocSize;
}

const char *ApiSpecificConfig::getRegistryPath() {
    return L0::registryPath;
}

} // namespace NEO
