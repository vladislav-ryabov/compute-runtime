/*
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const HardwareInfo DG2::hwInfo = Dg2HwConfig::hwInfo;

void DG2::adjustHardwareInfo(HardwareInfo *hwInfo) {}

void setupDG2HardwareInfoImpl(HardwareInfo *hwInfo, bool setupFeatureTableAndWorkaroundTable, uint64_t hwInfoConfig) {
    DG2::setupHardwareInfoBase(hwInfo, setupFeatureTableAndWorkaroundTable);
    Dg2HwConfig::setupHardwareInfo(hwInfo, setupFeatureTableAndWorkaroundTable);
}

void (*DG2::setupHardwareInfo)(HardwareInfo *, bool, const uint64_t) = setupDG2HardwareInfoImpl;
