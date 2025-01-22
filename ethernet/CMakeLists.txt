# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(EXTRA_ZEPHYR_MODULES /workdir/projects/external_modules/lvgl_veethree /workdir/projects/external_modules/usb_host_veethree /workdir/projects/external_modules/vg_lite_veethree)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
