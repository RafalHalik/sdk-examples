cmake_minimum_required(VERSION 3.13.1)

set(ZEPHYR_EXTRA_MODULES /home/user/C4.3_firmware/C4.3_workspace/modules/veethree_sim "${CMAKE_SOURCE_DIR}/../../modules/bsp" "${CMAKE_SOURCE_DIR}/../../modules/nafe")


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(uart_echo_bot)

set(MCUX_SDK_PROJECT_NAME app)

target_sources(app PRIVATE 
    src/main.c

    generated/events_init.c 
    generated/gui_guider.c 
    generated/setup_scr_screen.c 
    generated/widgets_init.c 
   
    generated/guider_fonts/lv_font_montserratMedium_10.c 
    generated/guider_fonts/lv_font_montserratMedium_16.c 
    generated/guider_fonts/lv_font_montserratMedium_25.c 
    generated/guider_fonts/lv_font_montserratMedium_22.c 
    generated/guider_fonts/lv_font_montserratMedium_50.c
    generated/images/_BACKGROUND_720x1280.c
)



target_include_directories(app PUBLIC 
    src    
    include
    generated
    generated/guider_customer_fonts   
    fatfs/source
    fatfs/source/fsl_usb_disk
)


target_compile_definitions(app PRIVATE LV_USE_GPU_NXP_VG_LITE)
target_compile_definitions(app PRIVATE SDK_OS_FREE_RTOS)
target_compile_definitions(app PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
