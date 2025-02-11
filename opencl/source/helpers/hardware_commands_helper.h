/*
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/helpers/per_thread_data.h"

#include <cstddef>
#include <cstdint>

namespace NEO {
class CommandQueue;
class Device;
class Kernel;
class LinearStream;
class IndirectHeap;
enum PreemptionMode : uint32_t;
struct CrossThreadInfo;
struct MultiDispatchInfo;

template <typename GfxFamily>
struct HardwareCommandsHelper : public PerThreadDataHelper {
    using WALKER_TYPE = typename GfxFamily::WALKER_TYPE;
    using BINDING_TABLE_STATE = typename GfxFamily::BINDING_TABLE_STATE;
    using RENDER_SURFACE_STATE = typename GfxFamily::RENDER_SURFACE_STATE;
    using INTERFACE_DESCRIPTOR_DATA = typename GfxFamily::INTERFACE_DESCRIPTOR_DATA;
    using MI_ATOMIC = typename GfxFamily::MI_ATOMIC;
    using COMPARE_OPERATION = typename GfxFamily::MI_SEMAPHORE_WAIT::COMPARE_OPERATION;

    static INTERFACE_DESCRIPTOR_DATA *getInterfaceDescriptor(
        const IndirectHeap &indirectHeap,
        uint64_t offsetInterfaceDescriptor,
        INTERFACE_DESCRIPTOR_DATA *inlineInterfaceDescriptor);

    inline static uint32_t additionalSizeRequiredDsh();

    static size_t sendInterfaceDescriptorData(
        const IndirectHeap &indirectHeap,
        uint64_t offsetInterfaceDescriptor,
        uint64_t kernelStartOffset,
        size_t sizeCrossThreadData,
        size_t sizePerThreadData,
        size_t bindingTablePointer,
        [[maybe_unused]] size_t offsetSamplerState,
        uint32_t numSamplers,
        const uint32_t threadGroupCount,
        uint32_t numThreadsPerThreadGroup,
        const Kernel &kernel,
        uint32_t bindingTablePrefetchSize,
        PreemptionMode preemptionMode,
        INTERFACE_DESCRIPTOR_DATA *inlineInterfaceDescriptor,
        const Device &device,
        WALKER_TYPE *walkerCmd);

    static void sendMediaStateFlush(
        LinearStream &commandStream,
        size_t offsetInterfaceDescriptorData);

    static void sendMediaInterfaceDescriptorLoad(
        LinearStream &commandStream,
        size_t offsetInterfaceDescriptorData,
        size_t sizeInterfaceDescriptorData);

    static size_t sendCrossThreadData(
        IndirectHeap &indirectHeap,
        Kernel &kernel,
        bool inlineDataProgrammingRequired,
        WALKER_TYPE *walkerCmd,
        uint32_t &sizeCrossThreadData);

    static size_t sendIndirectState(
        LinearStream &commandStream,
        IndirectHeap &dsh,
        IndirectHeap &ioh,
        IndirectHeap &ssh,
        Kernel &kernel,
        uint64_t kernelStartOffset,
        uint32_t simd,
        const size_t localWorkSize[3],
        const uint32_t threadGroupCount,
        const uint64_t offsetInterfaceDescriptorTable,
        uint32_t &interfaceDescriptorIndex,
        PreemptionMode preemptionMode,
        WALKER_TYPE *walkerCmd,
        INTERFACE_DESCRIPTOR_DATA *inlineInterfaceDescriptor,
        bool localIdsGenerationByRuntime,
        const Device &device);

    static void programPerThreadData(
        bool localIdsGenerationByRuntime,
        size_t &sizePerThreadData,
        size_t &sizePerThreadDataTotal,
        LinearStream &ioh,
        const Kernel &kernel,
        const size_t localWorkSize[3]);

    static size_t getSizeRequiredCS();
    static size_t getSizeRequiredForCacheFlush(const CommandQueue &commandQueue, const Kernel *kernel, uint64_t postSyncAddress);

    static size_t getSizeRequiredDSH(
        const Kernel &kernel);
    static size_t getSizeRequiredIOH(
        const Kernel &kernel,
        size_t localWorkSize = 256);
    static size_t getSizeRequiredSSH(
        const Kernel &kernel);

    static size_t getTotalSizeRequiredDSH(
        const MultiDispatchInfo &multiDispatchInfo);
    static size_t getTotalSizeRequiredIOH(
        const MultiDispatchInfo &multiDispatchInfo);
    static size_t getTotalSizeRequiredSSH(
        const MultiDispatchInfo &multiDispatchInfo);

    static void setInterfaceDescriptorOffset(
        WALKER_TYPE *walkerCmd,
        uint32_t &interfaceDescriptorIndex);

    static void programCacheFlushAfterWalkerCommand(LinearStream *commandStream, const CommandQueue &commandQueue, const Kernel *kernel, uint64_t postSyncAddress);

    static bool inlineDataProgrammingRequired(const Kernel &kernel);
    static bool kernelUsesLocalIds(const Kernel &kernel);
    static size_t checkForAdditionalBTAndSetBTPointer(IndirectHeap &ssh, const Kernel &kernel);
};
} // namespace NEO
