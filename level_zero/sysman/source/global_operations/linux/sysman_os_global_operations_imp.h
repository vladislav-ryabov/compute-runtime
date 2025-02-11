/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/os_interface/linux/pmt_util.h"

#include "level_zero/sysman/source/global_operations/sysman_os_global_operations.h"
#include "level_zero/sysman/source/linux/zes_os_sysman_imp.h"

namespace L0 {
namespace Sysman {
class SysfsAccess;

class LinuxGlobalOperationsImp : public OsGlobalOperations, NEO::NonCopyableOrMovableClass {
  public:
    bool getSerialNumber(char (&serialNumber)[ZES_STRING_PROPERTY_SIZE]) override;
    bool getBoardNumber(char (&boardNumber)[ZES_STRING_PROPERTY_SIZE]) override;
    void getBrandName(char (&brandName)[ZES_STRING_PROPERTY_SIZE]) override;
    void getModelName(char (&modelName)[ZES_STRING_PROPERTY_SIZE]) override;
    void getVendorName(char (&vendorName)[ZES_STRING_PROPERTY_SIZE]) override;
    void getDriverVersion(char (&driverVersion)[ZES_STRING_PROPERTY_SIZE]) override;
    void getWedgedStatus(zes_device_state_t *pState) override;
    void getRepairStatus(zes_device_state_t *pState) override;
    ze_result_t reset(ze_bool_t force) override;
    ze_result_t scanProcessesState(std::vector<zes_process_state_t> &pProcessList) override;
    ze_result_t deviceGetState(zes_device_state_t *pState) override;
    LinuxGlobalOperationsImp() = default;
    LinuxGlobalOperationsImp(OsSysman *pOsSysman);
    ~LinuxGlobalOperationsImp() override = default;

  protected:
    FsAccess *pFsAccess = nullptr;
    ProcfsAccess *pProcfsAccess = nullptr;
    SysfsAccess *pSysfsAccess = nullptr;
    LinuxSysmanImp *pLinuxSysmanImp = nullptr;
    int resetTimeout = 10000; // in milliseconds
    void releaseSysmanDeviceResources();
    void releaseDeviceResources();
    ze_result_t initDevice();
    void reInitSysmanDeviceResources();

  private:
    static const std::string deviceDir;
    static const std::string vendorFile;
    static const std::string subsystemVendorFile;
    static const std::string driverFile;
    static const std::string functionLevelReset;
    static const std::string clientsDir;
    static const std::string srcVersionFile;
    static const std::string agamaVersionFile;
    static const std::string ueventWedgedFile;
    bool getTelemOffsetAndTelemDir(uint64_t &telemOffset, const std::string &key, std::string &telemDir);
    std::string devicePciBdf = "";
    NEO::ExecutionEnvironment *executionEnvironment = nullptr;
    uint32_t rootDeviceIndex = 0u;
};

} // namespace Sysman
} // namespace L0
