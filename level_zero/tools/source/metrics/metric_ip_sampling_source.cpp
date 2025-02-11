/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/source/metrics/metric_ip_sampling_source.h"

#include "shared/source/debug_settings/debug_settings_manager.h"
#include "shared/source/helpers/hw_info.h"
#include "shared/source/helpers/string.h"

#include "level_zero/core/source/device/device.h"
#include "level_zero/core/source/device/device_imp.h"
#include "level_zero/tools/source/metrics/metric.h"
#include "level_zero/tools/source/metrics/metric_ip_sampling_streamer.h"
#include "level_zero/tools/source/metrics/os_interface_metric.h"
#include <level_zero/zet_api.h>

#include <cstring>

namespace L0 {
constexpr uint32_t ipSamplinMetricCount = 10u;
constexpr uint32_t ipSamplinDomainId = 100u;

std::unique_ptr<IpSamplingMetricSourceImp> IpSamplingMetricSourceImp::create(const MetricDeviceContext &metricDeviceContext) {
    return std::unique_ptr<IpSamplingMetricSourceImp>(new (std::nothrow) IpSamplingMetricSourceImp(metricDeviceContext));
}

IpSamplingMetricSourceImp::IpSamplingMetricSourceImp(const MetricDeviceContext &metricDeviceContext) : metricDeviceContext(metricDeviceContext) {
    metricIPSamplingpOsInterface = MetricIpSamplingOsInterface::create(metricDeviceContext.getDevice());
}

ze_result_t IpSamplingMetricSourceImp::getTimerResolution(uint64_t &resolution) {
    resolution = metricDeviceContext.getDevice().getNEODevice()->getDeviceInfo().outProfilingTimerClock;
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricSourceImp::getTimestampValidBits(uint64_t &validBits) {
    validBits = metricDeviceContext.getDevice().getNEODevice()->getHardwareInfo().capabilityTable.timestampValidBits;
    return ZE_RESULT_SUCCESS;
}

void IpSamplingMetricSourceImp::enable() {
    isEnabled = metricIPSamplingpOsInterface->isDependencyAvailable();
}

bool IpSamplingMetricSourceImp::isAvailable() {
    return isEnabled;
}

void IpSamplingMetricSourceImp::cacheMetricGroup() {

    if (metricDeviceContext.isImplicitScalingCapable()) {
        const auto deviceImp = static_cast<DeviceImp *>(&metricDeviceContext.getDevice());
        std::vector<IpSamplingMetricGroupImp *> subDeviceMetricGroup = {};
        subDeviceMetricGroup.reserve(deviceImp->subDevices.size());

        // Prepare cached metric group for sub-devices
        for (auto &subDevice : deviceImp->subDevices) {
            IpSamplingMetricSourceImp &source = subDevice->getMetricDeviceContext().getMetricSource<IpSamplingMetricSourceImp>();
            // 1 metric group available for IP Sampling
            uint32_t count = 1;
            zet_metric_group_handle_t hMetricGroup = {};
            const auto result = source.metricGroupGet(&count, &hMetricGroup);
            // Getting MetricGroup from sub-device cannot fail, since RootDevice is successful
            UNRECOVERABLE_IF(result != ZE_RESULT_SUCCESS);
            subDeviceMetricGroup.push_back(static_cast<IpSamplingMetricGroupImp *>(MetricGroup::fromHandle(hMetricGroup)));
        }

        cachedMetricGroup = MultiDeviceIpSamplingMetricGroupImp::create(subDeviceMetricGroup);
        return;
    }

    std::vector<IpSamplingMetricImp> metrics = {};
    metrics.reserve(ipSamplinMetricCount);

    zet_metric_properties_t metricProperties = {};

    metricProperties.stype = ZET_STRUCTURE_TYPE_METRIC_PROPERTIES;
    metricProperties.pNext = nullptr;
    strcpy_s(metricProperties.component, ZET_MAX_METRIC_COMPONENT, "XVE");
    metricProperties.tierNumber = 4;
    metricProperties.resultType = ZET_VALUE_TYPE_UINT64;

    // Preparing properties for IP seperately because of unique values
    strcpy_s(metricProperties.name, ZET_MAX_METRIC_NAME, "IP");
    strcpy_s(metricProperties.description, ZET_MAX_METRIC_DESCRIPTION, "IP address");
    metricProperties.metricType = ZET_METRIC_TYPE_IP_EXP;
    strcpy_s(metricProperties.resultUnits, ZET_MAX_METRIC_RESULT_UNITS, "Address");
    metrics.push_back(IpSamplingMetricImp(metricProperties));

    std::vector<std::pair<const char *, const char *>> metricPropertiesList = {
        {"Active", "Active cycles"},
        {"ControlStall", "Stall on control"},
        {"PipeStall", "Stall on pipe"},
        {"SendStall", "Stall on send"},
        {"DistStall", "Stall on distance"},
        {"SbidStall", "Stall on scoreboard"},
        {"SyncStall", "Stall on sync"},
        {"InstrFetchStall", "Stall on instruction fetch"},
        {"OtherStall", "Stall on other condition"},
    };

    // Preparing properties for others because of common values
    metricProperties.metricType = ZET_METRIC_TYPE_EVENT;
    strcpy_s(metricProperties.resultUnits, ZET_MAX_METRIC_RESULT_UNITS, "Events");

    for (auto &property : metricPropertiesList) {
        strcpy_s(metricProperties.name, ZET_MAX_METRIC_NAME, property.first);
        strcpy_s(metricProperties.description, ZET_MAX_METRIC_DESCRIPTION, property.second);
        metrics.push_back(IpSamplingMetricImp(metricProperties));
    }

    cachedMetricGroup = IpSamplingMetricGroupImp::create(*this, metrics);
    DEBUG_BREAK_IF(cachedMetricGroup == nullptr);
}

ze_result_t IpSamplingMetricSourceImp::metricGroupGet(uint32_t *pCount, zet_metric_group_handle_t *phMetricGroups) {

    if (!isEnabled) {
        *pCount = 0;
        return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
    }

    if (*pCount == 0) {
        *pCount = 1;
        return ZE_RESULT_SUCCESS;
    }

    if (cachedMetricGroup == nullptr) {
        cacheMetricGroup();
    }

    DEBUG_BREAK_IF(phMetricGroups == nullptr);
    phMetricGroups[0] = cachedMetricGroup->toHandle();
    *pCount = 1;

    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricSourceImp::appendMetricMemoryBarrier(CommandList &commandList) {
    return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

void IpSamplingMetricSourceImp::setMetricOsInterface(std::unique_ptr<MetricIpSamplingOsInterface> &metricIPSamplingpOsInterface) {
    this->metricIPSamplingpOsInterface = std::move(metricIPSamplingpOsInterface);
}

IpSamplingMetricGroupImp::IpSamplingMetricGroupImp(IpSamplingMetricSourceImp &metricSource,
                                                   std::vector<IpSamplingMetricImp> &metrics) : metricSource(metricSource) {
    this->metrics.reserve(metrics.size());
    for (const auto &metric : metrics) {
        this->metrics.push_back(std::make_unique<IpSamplingMetricImp>(metric));
    }

    properties.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    properties.pNext = nullptr;
    strcpy_s(properties.name, ZET_MAX_METRIC_GROUP_NAME, "EuStallSampling");
    strcpy_s(properties.description, ZET_MAX_METRIC_GROUP_DESCRIPTION, "EU stall sampling");
    properties.samplingType = ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED;
    properties.domain = ipSamplinDomainId;
    properties.metricCount = ipSamplinMetricCount;
}

ze_result_t IpSamplingMetricGroupImp::getProperties(zet_metric_group_properties_t *pProperties) {
    void *pNext = pProperties->pNext;
    *pProperties = properties;
    pProperties->pNext = pNext;

    if (pNext) {
        return getMetricGroupExtendedProperties(metricSource, pNext);
    }

    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricGroupImp::metricGet(uint32_t *pCount, zet_metric_handle_t *phMetrics) {

    if (*pCount == 0) {
        *pCount = static_cast<uint32_t>(metrics.size());
        return ZE_RESULT_SUCCESS;
    }
    // User is expected to allocate space.
    DEBUG_BREAK_IF(phMetrics == nullptr);

    *pCount = std::min(*pCount, static_cast<uint32_t>(metrics.size()));

    for (uint32_t i = 0; i < *pCount; i++) {
        phMetrics[i] = metrics[i]->toHandle();
    }

    return ZE_RESULT_SUCCESS;
}

bool IpSamplingMetricGroupImp::isMultiDeviceCaptureData(const size_t rawDataSize, const uint8_t *pRawData) {
    if (rawDataSize >= sizeof(IpSamplingMetricDataHeader)) {
        const auto header = reinterpret_cast<const IpSamplingMetricDataHeader *>(pRawData);
        return header->magic == IpSamplingMetricDataHeader::magicValue;
    }
    return false;
}

ze_result_t IpSamplingMetricGroupImp::calculateMetricValues(const zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                                            const uint8_t *pRawData, uint32_t *pMetricValueCount,
                                                            zet_typed_value_t *pMetricValues) {
    const bool calculateCountOnly = *pMetricValueCount == 0;

    if (isMultiDeviceCaptureData(rawDataSize, pRawData)) {
        NEO::printDebugString(NEO::DebugManager.flags.PrintDebugMessages.get(), stderr, "%s",
                              "INFO: The call is not supported for multiple devices\n"
                              "INFO: Please use zetMetricGroupCalculateMultipleMetricValuesExp instead\n");
        return ZE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (calculateCountOnly) {
        return getCalculatedMetricCount(rawDataSize, *pMetricValueCount);
    } else {
        return getCalculatedMetricValues(type, rawDataSize, pRawData, *pMetricValueCount, pMetricValues);
    }
}

ze_result_t IpSamplingMetricGroupImp::calculateMetricValuesExp(const zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                                               const uint8_t *pRawData, uint32_t *pSetCount,
                                                               uint32_t *pTotalMetricValueCount, uint32_t *pMetricCounts,
                                                               zet_typed_value_t *pMetricValues) {
    ze_result_t result = ZE_RESULT_SUCCESS;
    const bool calculateCountOnly = (*pTotalMetricValueCount == 0) || (*pSetCount == 0);
    if (calculateCountOnly) {
        *pTotalMetricValueCount = 0;
        *pSetCount = 0;
    }

    if (!isMultiDeviceCaptureData(rawDataSize, pRawData)) {
        result = this->calculateMetricValues(type, rawDataSize, pRawData, pTotalMetricValueCount, pMetricValues);
    } else {
        if (calculateCountOnly) {
            result = getCalculatedMetricCount(pRawData, rawDataSize, *pTotalMetricValueCount, 0);
        } else {
            result = getCalculatedMetricValues(type, rawDataSize, pRawData, *pTotalMetricValueCount, pMetricValues, 0);
        }
    }

    if ((result == ZE_RESULT_SUCCESS) || (result == ZE_RESULT_WARNING_DROPPED_DATA)) {
        *pSetCount = 1;
        if (!calculateCountOnly) {
            pMetricCounts[0] = *pTotalMetricValueCount;
        }
    } else {
        if (!calculateCountOnly) {
            pMetricCounts[0] = 0;
        }
    }

    return result;
}

ze_result_t getDeviceTimestamps(DeviceImp *deviceImp, const ze_bool_t synchronizedWithHost,
                                uint64_t *globalTimestamp, uint64_t *metricTimestamp) {

    ze_result_t result;
    uint64_t hostTimestamp;
    uint64_t deviceTimestamp;

    result = deviceImp->getGlobalTimestamps(&hostTimestamp, &deviceTimestamp);
    if (result != ZE_RESULT_SUCCESS) {
        *globalTimestamp = 0;
        *metricTimestamp = 0;
    } else {
        if (synchronizedWithHost) {
            *globalTimestamp = hostTimestamp;
        } else {
            *globalTimestamp = deviceTimestamp;
        }
        *metricTimestamp = deviceTimestamp;
        result = ZE_RESULT_SUCCESS;
    }

    return result;
}

ze_result_t IpSamplingMetricGroupImp::getMetricTimestampsExp(const ze_bool_t synchronizedWithHost,
                                                             uint64_t *globalTimestamp,
                                                             uint64_t *metricTimestamp) {
    DeviceImp *deviceImp = static_cast<DeviceImp *>(&getMetricSource().getMetricDeviceContext().getDevice());
    return getDeviceTimestamps(deviceImp, synchronizedWithHost, globalTimestamp, metricTimestamp);
}

ze_result_t IpSamplingMetricGroupImp::getCalculatedMetricCount(const size_t rawDataSize,
                                                               uint32_t &metricValueCount) {

    if ((rawDataSize % IpSamplingMetricGroupBase::rawReportSize) != 0) {
        return ZE_RESULT_ERROR_INVALID_SIZE;
    }

    const uint32_t rawReportCount = static_cast<uint32_t>(rawDataSize) / IpSamplingMetricGroupBase::rawReportSize;
    metricValueCount = rawReportCount * properties.metricCount;
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricGroupImp::getCalculatedMetricCount(const uint8_t *pMultiMetricData, const size_t rawDataSize, uint32_t &metricValueCount, const uint32_t setIndex) {

    // Iterate through headers and assign required sizes
    auto processedSize = 0u;
    while (processedSize < rawDataSize) {
        auto processMetricData = pMultiMetricData + processedSize;
        if (!isMultiDeviceCaptureData(rawDataSize - processedSize, processMetricData)) {
            return ZE_RESULT_ERROR_INVALID_SIZE;
        }

        auto header = reinterpret_cast<const IpSamplingMetricDataHeader *>(processMetricData);
        processedSize += sizeof(IpSamplingMetricDataHeader) + header->rawDataSize;
        if (header->setIndex != setIndex) {
            continue;
        }

        auto currTotalMetricValueCount = 0u;
        auto result = this->getCalculatedMetricCount(header->rawDataSize, currTotalMetricValueCount);
        if (result != ZE_RESULT_SUCCESS) {
            metricValueCount = 0;
            return result;
        }
        metricValueCount += currTotalMetricValueCount;
    }
    return ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricGroupImp::getCalculatedMetricValues(const zet_metric_group_calculation_type_t type, const size_t rawDataSize, const uint8_t *pMultiMetricData,
                                                                uint32_t &metricValueCount,
                                                                zet_typed_value_t *pCalculatedData, const uint32_t setIndex) {

    auto processedSize = 0u;
    auto isDataDropped = false;
    auto requestTotalMetricValueCount = metricValueCount;

    while (processedSize < rawDataSize && requestTotalMetricValueCount > 0) {
        auto processMetricData = pMultiMetricData + processedSize;
        if (!isMultiDeviceCaptureData(rawDataSize - processedSize, processMetricData)) {
            return ZE_RESULT_ERROR_INVALID_SIZE;
        }

        auto header = reinterpret_cast<const IpSamplingMetricDataHeader *>(processMetricData);
        processedSize += header->rawDataSize + sizeof(IpSamplingMetricDataHeader);
        if (header->setIndex != setIndex) {
            continue;
        }

        auto processMetricRawData = processMetricData + sizeof(IpSamplingMetricDataHeader);
        auto currTotalMetricValueCount = requestTotalMetricValueCount;
        auto result = this->calculateMetricValues(type, header->rawDataSize, processMetricRawData, &currTotalMetricValueCount, pCalculatedData);
        if (result != ZE_RESULT_SUCCESS) {
            if (result == ZE_RESULT_WARNING_DROPPED_DATA) {
                isDataDropped = true;
            } else {
                metricValueCount = 0;
                return result;
            }
        }
        pCalculatedData += currTotalMetricValueCount;
        requestTotalMetricValueCount -= currTotalMetricValueCount;
    }

    metricValueCount -= requestTotalMetricValueCount;
    return isDataDropped ? ZE_RESULT_WARNING_DROPPED_DATA : ZE_RESULT_SUCCESS;
}

ze_result_t IpSamplingMetricGroupImp::getCalculatedMetricValues(const zet_metric_group_calculation_type_t type, const size_t rawDataSize, const uint8_t *pRawData,
                                                                uint32_t &metricValueCount,
                                                                zet_typed_value_t *pCalculatedData) {
    bool dataOverflow = false;
    StallSumIpDataMap_t stallSumIpDataMap;

    // MAX_METRIC_VALUES is not supported yet.
    if (type != ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES) {
        return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
    }

    DEBUG_BREAK_IF(pCalculatedData == nullptr);

    const uint32_t rawReportSize = IpSamplingMetricGroupBase::rawReportSize;

    if ((rawDataSize % rawReportSize) != 0) {
        return ZE_RESULT_ERROR_INVALID_SIZE;
    }

    const uint32_t rawReportCount = static_cast<uint32_t>(rawDataSize) / rawReportSize;

    for (const uint8_t *pRawIpData = pRawData; pRawIpData < pRawData + (rawReportCount * rawReportSize); pRawIpData += rawReportSize) {
        dataOverflow |= stallIpDataMapUpdate(stallSumIpDataMap, pRawIpData);
    }

    metricValueCount = std::min<uint32_t>(metricValueCount, static_cast<uint32_t>(stallSumIpDataMap.size()) * properties.metricCount);
    std::vector<zet_typed_value_t> ipDataValues;
    uint32_t i = 0;
    for (auto it = stallSumIpDataMap.begin(); it != stallSumIpDataMap.end(); ++it) {
        stallSumIpDataToTypedValues(it->first, it->second, ipDataValues);
        for (auto jt = ipDataValues.begin(); (jt != ipDataValues.end()) && (i < metricValueCount); jt++, i++) {
            *(pCalculatedData + i) = *jt;
        }
        ipDataValues.clear();
    }

    return dataOverflow ? ZE_RESULT_WARNING_DROPPED_DATA : ZE_RESULT_SUCCESS;
}

/*
 * stall sample data item format:
 *
 * Bits		Field
 * 0  to 28	IP (addr)
 * 29 to 36	active count
 * 37 to 44	other count
 * 45 to 52	control count
 * 53 to 60	pipestall count
 * 61 to 68	send count
 * 69 to 76	dist_acc count
 * 77 to 84	sbid count
 * 85 to 92	sync count
 * 93 to 100	inst_fetch count
 *
 * bytes 49 and 50, subSlice
 * bytes 51 and 52, flags
 *
 * total size 64 bytes
 */
bool IpSamplingMetricGroupImp::stallIpDataMapUpdate(StallSumIpDataMap_t &stallSumIpDataMap, const uint8_t *pRawIpData) {

    const uint8_t *tempAddr = pRawIpData;
    uint64_t ip = 0ULL;
    memcpy_s(reinterpret_cast<uint8_t *>(&ip), sizeof(ip), tempAddr, sizeof(ip));
    ip &= 0x1fffffff;
    StallSumIpData_t &stallSumData = stallSumIpDataMap[ip];
    tempAddr += 3;

    auto getCount = [&tempAddr]() {
        uint16_t tempCount = 0;
        memcpy_s(reinterpret_cast<uint8_t *>(&tempCount), sizeof(tempCount), tempAddr, sizeof(tempCount));
        tempCount = (tempCount >> 5) & 0xff;
        tempAddr += 1;
        return static_cast<uint8_t>(tempCount);
    };

    stallSumData.activeCount += getCount();
    stallSumData.otherCount += getCount();
    stallSumData.controlCount += getCount();
    stallSumData.pipeStallCount += getCount();
    stallSumData.sendCount += getCount();
    stallSumData.distAccCount += getCount();
    stallSumData.sbidCount += getCount();
    stallSumData.syncCount += getCount();
    stallSumData.instFetchCount += getCount();

    struct StallCntrInfo {
        uint16_t subslice;
        uint16_t flags;
    } stallCntrInfo = {};

    tempAddr = pRawIpData + 48;
    memcpy_s(reinterpret_cast<uint8_t *>(&stallCntrInfo), sizeof(stallCntrInfo), tempAddr, sizeof(stallCntrInfo));

    constexpr int overflowDropFlag = (1 << 8);
    return stallCntrInfo.flags & overflowDropFlag;
}

// The order of push_back calls must match the order of metricPropertiesList.
void IpSamplingMetricGroupImp::stallSumIpDataToTypedValues(uint64_t ip,
                                                           StallSumIpData_t &sumIpData,
                                                           std::vector<zet_typed_value_t> &ipDataValues) {
    zet_typed_value_t tmpValueData;
    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = ip;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.activeCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.controlCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.pipeStallCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.sendCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.distAccCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.sbidCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.syncCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.instFetchCount;
    ipDataValues.push_back(tmpValueData);

    tmpValueData.type = ZET_VALUE_TYPE_UINT64;
    tmpValueData.value.ui64 = sumIpData.otherCount;
    ipDataValues.push_back(tmpValueData);
}

zet_metric_group_handle_t IpSamplingMetricGroupImp::getMetricGroupForSubDevice(const uint32_t subDeviceIndex) {
    return toHandle();
}

std::unique_ptr<IpSamplingMetricGroupImp> IpSamplingMetricGroupImp::create(IpSamplingMetricSourceImp &metricSource,
                                                                           std::vector<IpSamplingMetricImp> &ipSamplingMetrics) {
    return std::unique_ptr<IpSamplingMetricGroupImp>(new (std::nothrow) IpSamplingMetricGroupImp(metricSource, ipSamplingMetrics));
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::getProperties(zet_metric_group_properties_t *pProperties) {
    return subDeviceMetricGroup[0]->getProperties(pProperties);
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::metricGet(uint32_t *pCount, zet_metric_handle_t *phMetrics) {
    return subDeviceMetricGroup[0]->metricGet(pCount, phMetrics);
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::calculateMetricValues(const zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                                                       const uint8_t *pRawData, uint32_t *pMetricValueCount,
                                                                       zet_typed_value_t *pMetricValues) {
    return subDeviceMetricGroup[0]->calculateMetricValues(type, rawDataSize, pRawData, pMetricValueCount, pMetricValues);
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::calculateMetricValuesExp(const zet_metric_group_calculation_type_t type, size_t rawDataSize,
                                                                          const uint8_t *pRawData, uint32_t *pSetCount,
                                                                          uint32_t *pTotalMetricValueCount, uint32_t *pMetricCounts,
                                                                          zet_typed_value_t *pMetricValues) {

    const bool calculateCountOnly = *pSetCount == 0 || *pTotalMetricValueCount == 0;
    bool isDroppedData = false;
    ze_result_t result = ZE_RESULT_SUCCESS;

    if (calculateCountOnly) {
        *pSetCount = 0;
        *pTotalMetricValueCount = 0;
        for (uint32_t setIndex = 0; setIndex < subDeviceMetricGroup.size(); setIndex++) {
            uint32_t currTotalMetricValueCount = 0;
            result = subDeviceMetricGroup[setIndex]->getCalculatedMetricCount(pRawData, rawDataSize, currTotalMetricValueCount, setIndex);
            if (result != ZE_RESULT_SUCCESS) {
                return result;
            }
            *pTotalMetricValueCount += currTotalMetricValueCount;
        }
        *pSetCount = static_cast<uint32_t>(subDeviceMetricGroup.size());
    } else {
        memset(pMetricCounts, 0, *pSetCount);
        const auto maxSets = std::min<uint32_t>(static_cast<uint32_t>(subDeviceMetricGroup.size()), *pSetCount);

        auto tempTotalMetricValueCount = *pTotalMetricValueCount;
        for (uint32_t setIndex = 0; setIndex < maxSets; setIndex++) {
            uint32_t currTotalMetricValueCount = tempTotalMetricValueCount;
            result = subDeviceMetricGroup[setIndex]->getCalculatedMetricValues(type, rawDataSize, pRawData, currTotalMetricValueCount, pMetricValues, setIndex);
            if (result != ZE_RESULT_SUCCESS) {
                if (result == ZE_RESULT_WARNING_DROPPED_DATA) {
                    isDroppedData = true;
                } else {
                    memset(pMetricCounts, 0, *pSetCount);
                    return result;
                }
            }

            pMetricCounts[setIndex] = currTotalMetricValueCount;
            pMetricValues += currTotalMetricValueCount;
            tempTotalMetricValueCount -= currTotalMetricValueCount;
        }
        *pTotalMetricValueCount -= tempTotalMetricValueCount;
    }

    return isDroppedData ? ZE_RESULT_WARNING_DROPPED_DATA : ZE_RESULT_SUCCESS;
}

zet_metric_group_handle_t MultiDeviceIpSamplingMetricGroupImp::getMetricGroupForSubDevice(const uint32_t subDeviceIndex) {
    return subDeviceMetricGroup[subDeviceIndex]->toHandle();
}

void MultiDeviceIpSamplingMetricGroupImp::closeSubDeviceStreamers(std::vector<IpSamplingMetricStreamerImp *> &subDeviceStreamers) {
    for (auto streamer : subDeviceStreamers) {
        streamer->close();
    }
}

ze_result_t MultiDeviceIpSamplingMetricGroupImp::getMetricTimestampsExp(const ze_bool_t synchronizedWithHost,
                                                                        uint64_t *globalTimestamp,
                                                                        uint64_t *metricTimestamp) {
    DeviceImp *deviceImp = static_cast<DeviceImp *>(&subDeviceMetricGroup[0]->getMetricSource().getMetricDeviceContext().getDevice());
    return getDeviceTimestamps(deviceImp, synchronizedWithHost, globalTimestamp, metricTimestamp);
}

std::unique_ptr<MultiDeviceIpSamplingMetricGroupImp> MultiDeviceIpSamplingMetricGroupImp::create(
    std::vector<IpSamplingMetricGroupImp *> &subDeviceMetricGroup) {
    UNRECOVERABLE_IF(subDeviceMetricGroup.size() == 0);
    return std::unique_ptr<MultiDeviceIpSamplingMetricGroupImp>(new (std::nothrow) MultiDeviceIpSamplingMetricGroupImp(subDeviceMetricGroup));
}

IpSamplingMetricImp::IpSamplingMetricImp(zet_metric_properties_t &properties) : properties(properties) {
}

ze_result_t IpSamplingMetricImp::getProperties(zet_metric_properties_t *pProperties) {
    *pProperties = properties;
    return ZE_RESULT_SUCCESS;
}

template <>
IpSamplingMetricSourceImp &MetricDeviceContext::getMetricSource<IpSamplingMetricSourceImp>() const {
    return static_cast<IpSamplingMetricSourceImp &>(*metricSources.at(MetricSource::SourceType::IpSampling));
}

} // namespace L0
