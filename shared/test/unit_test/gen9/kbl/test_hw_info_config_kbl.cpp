/*
 * Copyright (C) 2018-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gen9/kbl/device_ids_configs_kbl.h"
#include "shared/source/os_interface/hw_info_config.h"
#include "shared/test/common/helpers/default_hw_info.h"
#include "shared/test/common/test_macros/hw_test.h"

#include "platforms.h"

using namespace NEO;

TEST(KblHwInfoConfig, GivenIncorrectDataWhenConfiguringHwInfoThenErrorIsReturned) {
    if (IGFX_KABYLAKE != productFamily) {
        return;
    }
    HardwareInfo hwInfo = *defaultHwInfo;
    GT_SYSTEM_INFO &gtSystemInfo = hwInfo.gtSystemInfo;

    uint64_t config = 0xdeadbeef;
    gtSystemInfo = {0};
    EXPECT_ANY_THROW(hardwareInfoSetup[productFamily](&hwInfo, false, config));
    EXPECT_EQ(0u, gtSystemInfo.SliceCount);
    EXPECT_EQ(0u, gtSystemInfo.SubSliceCount);
    EXPECT_EQ(0u, gtSystemInfo.DualSubSliceCount);
    EXPECT_EQ(0u, gtSystemInfo.EUCount);
}

using KblHwInfo = ::testing::Test;

KBLTEST_F(KblHwInfo, givenBoolWhenCallKblHardwareInfoSetupThenFeatureTableAndWorkaroundTableAreSetCorrect) {
    uint64_t configs[] = {
        0x100030008,
        0x200030008,
        0x300030008,
        0x100020006,
        0x100030006};
    bool boolValue[]{
        true, false};
    HardwareInfo hwInfo = *defaultHwInfo;
    GT_SYSTEM_INFO &gtSystemInfo = hwInfo.gtSystemInfo;
    FeatureTable &featureTable = hwInfo.featureTable;
    WorkaroundTable &workaroundTable = hwInfo.workaroundTable;
    PLATFORM &platform = hwInfo.platform;

    for (auto &config : configs) {
        for (auto setParamBool : boolValue) {

            gtSystemInfo = {0};
            featureTable = {};
            workaroundTable = {};
            platform.usRevId = 9;
            hardwareInfoSetup[productFamily](&hwInfo, setParamBool, config);

            EXPECT_EQ(setParamBool, featureTable.flags.ftrGpGpuMidBatchPreempt);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrGpGpuThreadGroupLevelPreempt);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrL3IACoherency);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrGpGpuMidThreadLevelPreempt);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrPPGTT);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrSVM);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrIA32eGfxPTEs);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrDisplayYTiling);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrTranslationTable);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrUserModeTranslationTable);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrFbc);
            EXPECT_EQ(setParamBool, featureTable.flags.ftrTileY);

            EXPECT_EQ(setParamBool, workaroundTable.flags.waSendMIFLUSHBeforeVFE);
            EXPECT_EQ(setParamBool, workaroundTable.flags.waMsaa8xTileYDepthPitchAlignment);
            EXPECT_EQ(setParamBool, workaroundTable.flags.waLosslessCompressionSurfaceStride);
            EXPECT_EQ(setParamBool, workaroundTable.flags.waFbcLinearSurfaceStride);
            EXPECT_EQ(setParamBool, workaroundTable.flags.wa4kAlignUVOffsetNV12LinearSurface);
            EXPECT_EQ(setParamBool, workaroundTable.flags.waSamplerCacheFlushBetweenRedescribedSurfaceReads);
            EXPECT_EQ(false, workaroundTable.flags.waDisableLSQCROPERFforOCL);
            EXPECT_EQ(false, workaroundTable.flags.waEncryptedEdramOnlyPartials);

            platform.usRevId = 1;
            workaroundTable = {};
            hardwareInfoSetup[productFamily](&hwInfo, true, config);

            EXPECT_EQ(true, workaroundTable.flags.waDisableLSQCROPERFforOCL);
            EXPECT_EQ(true, workaroundTable.flags.waEncryptedEdramOnlyPartials);
        }
    }
}

KBLTEST_F(KblHwInfo, givenHwInfoConfigWhenGetProductConfigThenCorrectMatchIsFound) {
    HardwareInfo hwInfo = *defaultHwInfo;
    const auto &hwInfoConfig = *HwInfoConfig::get(hwInfo.platform.eProductFamily);
    for (const auto &deviceId : amlDeviceIds) {
        hwInfo.platform.usDeviceID = deviceId;
        EXPECT_EQ(hwInfoConfig.getProductConfigFromHwInfo(hwInfo), AOT::AML);
    }
    for (const auto &deviceId : kblDeviceIds) {
        hwInfo.platform.usDeviceID = deviceId;
        EXPECT_EQ(hwInfoConfig.getProductConfigFromHwInfo(hwInfo), AOT::KBL);
    }

    hwInfo.platform.usDeviceID = 0u;
    EXPECT_EQ(hwInfoConfig.getProductConfigFromHwInfo(hwInfo), AOT::UNKNOWN_ISA);
}

KBLTEST_F(KblHwInfo, givenHwInfoConfigWhenGettingEvictIfNecessaryFlagSupportedThenExpectTrue) {
    HardwareInfo hwInfo = *defaultHwInfo;
    const auto &hwInfoConfig = *HwInfoConfig::get(hwInfo.platform.eProductFamily);
    EXPECT_TRUE(hwInfoConfig.isEvictionIfNecessaryFlagSupported());
}

KBLTEST_F(KblHwInfo, givenHwInfoConfigWhenGetCommandsStreamPropertiesSupportThenExpectCorrectValues) {
    HardwareInfo hwInfo = *defaultHwInfo;
    const auto &hwInfoConfig = *HwInfoConfig::get(hwInfo.platform.eProductFamily);

    EXPECT_TRUE(hwInfoConfig.getScmPropertyThreadArbitrationPolicySupport());
    EXPECT_FALSE(hwInfoConfig.getScmPropertyCoherencyRequiredSupport());
    EXPECT_FALSE(hwInfoConfig.getScmPropertyZPassAsyncComputeThreadLimitSupport());
    EXPECT_FALSE(hwInfoConfig.getScmPropertyPixelAsyncComputeThreadLimitSupport());
    EXPECT_FALSE(hwInfoConfig.getScmPropertyLargeGrfModeSupport());
    EXPECT_FALSE(hwInfoConfig.getScmPropertyDevicePreemptionModeSupport());

    EXPECT_FALSE(hwInfoConfig.getSbaPropertyGlobalAtomicsSupport());
    EXPECT_TRUE(hwInfoConfig.getSbaPropertyStatelessMocsSupport());

    EXPECT_TRUE(hwInfoConfig.getFrontEndPropertyScratchSizeSupport());
    EXPECT_FALSE(hwInfoConfig.getFrontEndPropertyPrivateScratchSizeSupport());

    EXPECT_TRUE(hwInfoConfig.getPreemptionDbgPropertyPreemptionModeSupport());
    EXPECT_TRUE(hwInfoConfig.getPreemptionDbgPropertyStateSipSupport());
    EXPECT_TRUE(hwInfoConfig.getPreemptionDbgPropertyCsrSurfaceSupport());

    EXPECT_FALSE(hwInfoConfig.getFrontEndPropertyComputeDispatchAllWalkerSupport());
    EXPECT_FALSE(hwInfoConfig.getFrontEndPropertyDisableEuFusionSupport());
    EXPECT_FALSE(hwInfoConfig.getFrontEndPropertyDisableOverDispatchSupport());
    EXPECT_FALSE(hwInfoConfig.getFrontEndPropertySingleSliceDispatchCcsModeSupport());

    EXPECT_TRUE(hwInfoConfig.getPipelineSelectPropertyModeSelectedSupport());
    EXPECT_TRUE(hwInfoConfig.getPipelineSelectPropertyMediaSamplerDopClockGateSupport());
    EXPECT_FALSE(hwInfoConfig.getPipelineSelectPropertySystolicModeSupport());
}
