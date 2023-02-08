/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gen12lp/hw_cmds_adln.h"
#include "shared/source/gen12lp/hw_info_adln.h"
#include "shared/source/helpers/hw_info.h"
#include "shared/source/os_interface/hw_info_config.h"
#include "shared/source/os_interface/hw_info_config.inl"
#include "shared/source/os_interface/hw_info_config_bdw_and_later.inl"

#include "platforms.h"

constexpr static auto gfxProduct = IGFX_ALDERLAKE_N;

#include "shared/source/gen12lp/adln/os_agnostic_hw_info_config_adln.inl"
#include "shared/source/gen12lp/os_agnostic_hw_info_config_gen12lp.inl"

template class NEO::ProductHelperHw<gfxProduct>;
