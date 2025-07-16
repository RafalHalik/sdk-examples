cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_echo_bot)


target_sources(app PRIVATE 
    
    src/main.c
    src/host_msd_fatfs.c

    fatfs/source/fsl_usb_disk/fsl_usb_disk_bm.c 
    fatfs/source/diskio.c
    fatfs/source/ff.c 
    fatfs/source/ffsystem.c 
    fatfs/source/ffunicode.c
    )



target_include_directories(app PUBLIC 
    include
    src  
    fatfs/source
    fatfs/source/fsl_usb_disk
    )


target_compile_definitions(app PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
