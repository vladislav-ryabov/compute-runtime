/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/command_container/command_encoder.h"
#include "shared/source/helpers/cache_policy.h"
#include "shared/source/kernel/kernel_descriptor.h"
#include "shared/source/program/kernel_info.h"
#include "shared/test/common/fixtures/device_fixture.h"
#include "shared/test/common/test_macros/test.h"

namespace NEO {

class CommandEncodeStatesFixture : public DeviceFixture {
  public:
    class MyMockCommandContainer : public CommandContainer {
      public:
        using CommandContainer::dirtyHeaps;
        using CommandContainer::indirectHeaps;

        IndirectHeap *getHeapWithRequiredSizeAndAlignment(HeapType heapType, size_t sizeRequired, size_t alignment) override {
            getHeapWithRequiredSizeAndAlignmentCalled++;
            return CommandContainer::getHeapWithRequiredSizeAndAlignment(heapType, sizeRequired, alignment);
        }
        uint32_t getHeapWithRequiredSizeAndAlignmentCalled = 0u;
    };

    void setUp();
    void tearDown();

    EncodeDispatchKernelArgs createDefaultDispatchKernelArgs(Device *device,
                                                             DispatchKernelEncoderI *dispatchInterface,
                                                             const void *threadGroupDimensions,
                                                             bool requiresUncachedMocs);

    template <typename FamilyType>
    EncodeStateBaseAddressArgs<FamilyType> createDefaultEncodeStateBaseAddressArgs(
        CommandContainer *container,
        typename FamilyType::STATE_BASE_ADDRESS &sbaCmd,
        uint32_t statelessMocs) {

        EncodeStateBaseAddressArgs<FamilyType> args = {
            container,                                // container
            sbaCmd,                                   // sbaCmd
            nullptr,                                  // sbaProperties
            statelessMocs,                            // statelessMocsIndex
            l1CachePolicyData.getL1CacheValue(false), // l1CachePolicy
            l1CachePolicyData.getL1CacheValue(true),  // l1CachePolicyDebuggerActive
            false,                                    // useGlobalAtomics
            false,                                    // multiOsContextCapable
            false,                                    // isRcs
            container->doubleSbaWaRef()};             // doubleSbaWa
        return args;
    }

    KernelDescriptor descriptor;
    KernelInfo kernelInfo;
    std::unique_ptr<MyMockCommandContainer> cmdContainer;
    NEO::L1CachePolicy l1CachePolicyData;
};

} // namespace NEO

struct WalkerThreadFixture {
    void setUp();
    void tearDown() {}

    uint32_t startWorkGroup[3];
    uint32_t numWorkGroups[3];
    uint32_t workGroupSizes[3];
    uint32_t simd;
    uint32_t localIdDimensions;
    uint32_t requiredWorkGroupOrder;
};

using WalkerThreadTest = Test<WalkerThreadFixture>;
