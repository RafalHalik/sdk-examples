# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(BOARD_ROOT /workdir/extra_boards)
set(EXTRA_ZEPHYR_MODULES /workdir/projects/external_modules/lvgl_veethree /workdir/projects/external_modules/usb_host_veethree /workdir/projects/external_modules/vg_lite_veethree)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE src/main.c)
target_compile_definitions(app PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
