# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
 
set(ZEPHYR_EXTRA_MODULES /home/user/C4.3_firmware/C4.3_workspace/modules/veethree_sim "${CMAKE_SOURCE_DIR}/../../modules/bsp" "${CMAKE_SOURCE_DIR}/../../modules/nafe")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
