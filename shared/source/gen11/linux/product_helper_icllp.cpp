/*
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gen11/hw_cmds_icllp.h"
#include "shared/source/helpers/hw_info.h"
#include "shared/source/os_interface/product_helper.h"
#include "shared/source/os_interface/product_helper.inl"
#include "shared/source/os_interface/product_helper_bdw_and_later.inl"

constexpr static auto gfxProduct = IGFX_ICELAKE_LP;
#include "shared/source/gen11/icllp/os_agnostic_product_helper_icllp.inl"
namespace NEO {

template <>
int ProductHelperHw<gfxProduct>::configureHardwareCustom(HardwareInfo *hwInfo, OSInterface *osIface) const {
    if (nullptr == osIface) {
        return 0;
    }

    GT_SYSTEM_INFO *gtSystemInfo = &hwInfo->gtSystemInfo;

    gtSystemInfo->SliceCount = 1;

    return 0;
}

template class ProductHelperHw<gfxProduct>;
} // namespace NEO
