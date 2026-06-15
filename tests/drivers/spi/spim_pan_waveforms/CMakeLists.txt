#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})


if(CONFIG_SPIM_WORKAROUND_FORCE_OFF)
  if(CONFIG_SOC_NRF54L15)
    zephyr_compile_definitions(NRF54L_ERRATA_8_ENABLE_WORKAROUND=0)
  elseif(CONFIG_SOC_NRF54H20)
    zephyr_compile_definitions(NRF54H_ERRATA_115_ENABLE_WORKAROUND=0)
   endif()
endif()

project(spim_pan_waveforms)

target_sources(app PRIVATE src/main.c)
