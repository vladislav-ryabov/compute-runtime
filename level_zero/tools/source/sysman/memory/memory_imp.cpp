/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/source/sysman/memory/memory_imp.h"

#include "level_zero/tools/source/sysman/sysman_imp.h"

namespace L0 {

ze_result_t MemoryImp::memoryGetBandwidth(zes_mem_bandwidth_t *pBandwidth) {
    return pOsMemory->getBandwidth(pBandwidth);
}

ze_result_t MemoryImp::memoryGetState(zes_mem_state_t *pState) {
    return pOsMemory->getState(pState);
}

ze_result_t MemoryImp::memoryGetProperties(zes_mem_properties_t *pProperties) {
    *pProperties = memoryProperties;
    return ZE_RESULT_SUCCESS;
}

ze_result_t MemoryImp::memoryGetBandwidthEx(uint64_t *pReadCounters, uint64_t *pWriteCounters, uint64_t *pMaxBandwidth, uint64_t timeout) {
    return pOsMemory->getBandwidthEx(pReadCounters, pWriteCounters, pMaxBandwidth, timeout);
}

void MemoryImp::init() {
    this->initSuccess = pOsMemory->isMemoryModuleSupported();
    if (this->initSuccess == true) {
        pOsMemory->getProperties(&memoryProperties);
    }
}

MemoryImp::MemoryImp(OsSysman *pOsSysman, ze_device_handle_t handle) {
    uint32_t subdeviceId = 0;
    ze_bool_t onSubdevice = false;
    SysmanDeviceImp::getSysmanDeviceInfo(handle, subdeviceId, onSubdevice, true);
    pOsMemory = OsMemory::create(pOsSysman, onSubdevice, subdeviceId);
    init();
}

MemoryImp::~MemoryImp() = default;

} // namespace L0
