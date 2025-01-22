/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// flash includes...

#include <stdio.h>

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/display.h>
#include <zephyr/irq.h>

#include "fsl_device_registers.h"
#include "fsl_common.h"

#include "../generated/gui_guider.h"
#include "../generated/events_init.h"


#ifdef CONFIG_VG_LITE_VEETHREE
#include "/workdir/projects/external_modules/vg_lite_veethree/include/lvgl_support.h"
#endif
#include "/workdir/modules/hal/nxp/mcux/mcux-sdk/devices/MIMXRT1166/MIMXRT1166_cm7_features.h"

#define VERSION_NUM 45

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                                              {0});

#define SEM_COUNT_INITAL 0
#define SEM_COUNT_MAX 1


/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

lv_anim_t a;
lv_ui Guider_ui;

static void anim_x_cb(void *var, int32_t v);
void lv_example_anim_2(void);
static void anim_size_cb(void *var, int32_t v);


int main(void)
{
    char buffer[100];
    uint64_t start_time, end_time = 0;
    uint32_t fps = 0;
    
   
    setup_ui(&Guider_ui);
    events_init(&Guider_ui);
    lv_example_anim_2();

    uint8_t i = 0;
    start_time = k_uptime_get();
    while (1)
    {        
        lv_task_handler();   
       
        fps++;
        end_time = k_uptime_get();
        if ((end_time - start_time) >= 1000)
        {            
            fps = 0;
            start_time = k_uptime_get();
        }        
       
        k_msleep(20);
    }

    return 0;
}

static void anim_x_cb(void *var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void *var, int32_t v)
{
    lv_bar_set_value(Guider_ui.screen_bar_1, v, LV_ANIM_OFF);
    lv_bar_set_value(Guider_ui.screen_bar_2, v, LV_ANIM_OFF);
    lv_bar_set_value(Guider_ui.screen_bar_3, v, LV_ANIM_OFF);
    lv_bar_set_value(Guider_ui.screen_bar_4, v, LV_ANIM_OFF);

    lv_arc_set_value(Guider_ui.screen_right_btm_arc, v);
    lv_arc_set_value(Guider_ui.screen_left_top_arc, v);
    lv_arc_set_value(Guider_ui.screen_right_top_arc, v);
    lv_arc_set_value(Guider_ui.screen_left_btm_arc, v);

    lv_meter_set_indicator_value(Guider_ui.screen_meter_1, Guider_ui.screen_meter_1_scale_0_ndline_0, (v * 50));
}

/**
 * Create a playback animation
 */
void lv_example_anim_2(void)
{
    lv_anim_init(&a);
    lv_anim_set_var(&a, &Guider_ui);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_time(&a, 2000);
    lv_anim_set_playback_delay(&a, 100);
    lv_anim_set_playback_time(&a, 800);
    lv_anim_set_repeat_delay(&a, 900);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_set_exec_cb(&a, anim_size_cb);
    lv_anim_start(&a);
   
}

