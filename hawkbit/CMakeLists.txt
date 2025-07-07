# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(EXTRA_ZEPHYR_MODULES /workdir/extra_drivers/drivers/nafe13388;/workdir/extra_drivers/drivers/drv8844;/workdir/extra_drivers/drivers/flash)
set(BOARD_ROOT /workdir/extra_boards )
set(DTS_ROOT /workdir/extra_dts)
# This application has its own Kconfig options.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(<REPLACE-ME>)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_NET_DHCPV4 app PRIVATE src/dhcp.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/self_sign.der
    ${gen_dir}/self_sign.der.inc
    )
