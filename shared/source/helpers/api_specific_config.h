/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <cstdint>
#include <string>

namespace NEO {
struct ApiSpecificConfig {
    enum ApiType { OCL,
                   L0 };
    static bool isStatelessCompressionSupported();
    static bool isBcsSplitWaSupported();
    static bool getGlobalBindlessHeapConfiguration();
    static bool getBindlessMode();
    static bool isDeviceAllocationCacheEnabled();
    static bool isDynamicPostSyncAllocLayoutEnabled();
    static bool isRelaxedOrderingEnabled();
    static ApiType getApiType();
    static std::string getName();
    static uint64_t getReducedMaxAllocSize(uint64_t maxAllocSize);
    static const char *getRegistryPath();

    static std::string getAubPrefixForSpecificApi() {
        return (getName() + "_");
    }
};
} // namespace NEO
