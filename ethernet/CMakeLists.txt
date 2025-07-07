# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(EXTRA_ZEPHYR_MODULES /workdir/extra_drivers/drivers/nafe13388;/workdir/extra_drivers/drivers/drv8844;/workdir/extra_drivers/drivers/flash)
set(BOARD_ROOT /workdir/extra_boards )
set(DTS_ROOT /workdir/extra_dts)
 
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
