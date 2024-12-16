/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// flash includes...

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
// #include <lvgl_input_device.h>
#include <zephyr/irq.h>

#include "board.h"
#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "fsl_common.h"

#include "../generated/gui_guider.h"
#include "../generated/events_init.h"

// Display
#include <zephyr/drivers/display.h>
#include <lvgl.h>
// #include <lv_demos.h>

#include "usb_phy.h"
#include "usb_host_msd.h"
#include "host_msd_fatfs.h"
#include "app.h"
#ifdef CONFIG_VG_LITE_VEETHREE
#include "/workdir/projects/external_modules/vg_lite_veethree/include/lvgl_support.h"
#endif
#include "zephyr/dfu/mcuboot.h"
#include "/workdir/modules/hal/nxp/mcux/mcux-sdk/devices/MIMXRT1166/MIMXRT1166_cm7_features.h"

#define VERSION_NUM 45

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                                              {0});

#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

#define SEM_COUNT_INITAL 0
#define SEM_COUNT_MAX 1

#define USB_THREAD_STACK_SIZE 2048

#define USB_THREAD_PRIOROTY 7
#define LVGL_THREAD_PRIOROTY 8
#define USB_UPDATE_THREAD_PRIOROTY 2

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

K_THREAD_STACK_DEFINE(usb_thread_stack, USB_THREAD_STACK_SIZE);

K_THREAD_STACK_DEFINE(lvgl_stack_area, 2048);



// K_THREAD_STACK_DEFINE(lvgl_thread_stack, USB_THREAD_STACK_SIZE);
// struct k_thread lvgl_thread_data;

// K_THREAD_STACK_DEFINE(usb_update_thread_stack, USB_THREAD_STACK_SIZE);
// struct k_thread usb_update_thread_data;

/*! @brief USB host msd fatfs instance global variable */
extern usb_host_msd_fatfs_instance_t g_MsdFatfsInstance;
usb_host_handle g_HostHandle;

struct k_thread usb_thread_data;

/* Run USB stack on a separate thread */
static k_tid_t usb_thread_id;

static k_tid_t lvgl_thread_id;

static k_tid_t usb_update_thread_id;

lv_anim_t a;

static lv_style_t style_indic;
static lv_obj_t *count_label;
static lv_obj_t *update_label;
static uint32_t count;

static lv_obj_t *mainScreen;
static lv_obj_t *updateScreen;
static lv_obj_t *bar1;

static bool stopReadingUsb = false;

char count_str[11] = {0};
bool updateInProgress = false;

bool usb_attached = false;
lv_ui guider_ui;

struct device *display_dev;


void usb_thread(void *p1, void *p2, void *p3);

void lvgl_thread(void *p1, void *p2, void *p3);

void usb_update_thread(void *p1, void *p2, void *p3);

void start_update_process(bool updating);

static void anim_x_cb(void *var, int32_t v);
void lv_example_anim_2(void);
void lv_example_anim_3(void);
void lv_example_anim_4(void);
static void anim_size_cb(void *var, int32_t v);

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

static void USB_HostApplicationInit(void);

void start_update_process(bool updating)
{
    updateInProgress = updating;
}

void dynamic_usb_isr(const void *param)
{
    USB_HostEhciIsrFunction(g_HostHandle);
}

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>

// #include "usbh_ch9.h"
// #include "usbh_device.h"



int main(void)
{
    char buffer[100];
    uint64_t start_time, end_time = 0;
    uint32_t fps = 0;
    // k_tid_t my_tid;              Task init stuff 
    // struct k_thread task_handle;
 
    // lv_port_disp_init();
   
    setup_ui(&guider_ui);
    events_init(&guider_ui);

    // lvgl_setup();
    USB_HostApplicationInit();
    
   

    printf("\r\n\r\n**************\r\n VERSION %d \r\n**************\r\n\r\n", VERSION_NUM);

    lv_example_anim_2();
    uint8_t i = 0;
    start_time = k_uptime_get();
    while (1)
    {        
        lv_task_handler();

        USB_HostTaskFn(g_HostHandle); 
        USB_HostMsdTask(&g_MsdFatfsInstance);  
        if(updateInProgress == true)
        {
            usb_update_task();  // Handles usb reading and writing new img to flash 
        }
       
        fps++;
        end_time = k_uptime_get();
        if ((end_time - start_time) >= 1000)
        {
            //printf("fps %d \r\n", fps);
            fps = 0;
            start_time = k_uptime_get();
        }        
       
        k_msleep(20);
    }

    return 0;
}

void usb_thread(void *p1, void *p2, void *p3)
{
    LOG_INF("USB Thread started\r\n");
    while (1)
    {
        USB_HostTaskFn(g_HostHandle);
        USB_HostMsdTask(&g_MsdFatfsInstance);
        usb_update_task();
        if (updateInProgress == false)
        {
            k_msleep(10);
        }
    }
}

void usb_update_thread(void *p1, void *p2, void *p3)
{
    LOG_INF("USB update Thread started\r\n");

    while (1)
    {
        usb_update_task();
        k_msleep(1);
    }
}

void lvgl_thread(void *p1, void *p2, void *p3)
{
    char buffer[100];

    LOG_INF("LVGL Thread started\r\n");
    // lvgl_setup();

    lv_obj_t *mainScreen = lv_scr_act();

    while (1)
    {

        count++;
        sprintf(buffer, "%d", count);
        lv_label_set_text(count_label, buffer);

        if (updateInProgress)
        {
            LOG_INF("SWITCH TO UPDATE SCREEN \r\n");
            if (lv_scr_act() != updateScreen)
            {
                lv_scr_load(updateScreen);
                lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
                lv_obj_add_style(bar1, &style_indic, LV_PART_INDICATOR);
                lv_label_set_text(update_label, "USB update in progress...");
                lv_bar_set_value(bar1, 0, LV_ANIM_OFF);
                LOG_INF("SWITCH TO UPDATE SCREEN \r\n");
            }
        }
        else
        {
            if (lv_scr_act() != mainScreen)
            {
                lv_scr_load(mainScreen);
                LOG_INF("SWITCH TO MAIN SCREEN \r\n");
            }
        }

        lv_task_handler();
        k_msleep(100);
    }
}

/*******************************************************************************/
void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0) // 480000000UL
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000); // 24000000
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000);          // 24000000
    }
    else
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000); // 24000000
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000);          // 24000000
    }
    usb_status_t stat = USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
    usb_echo("stat %d", stat);
}

void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHS_IRQS;
    irqNumber = usbHOSTEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

    irq_connect_dynamic(136, USB_HOST_INTERRUPT_PRIORITY, dynamic_usb_isr, NULL, 0);
    irq_enable(136);
}

/*******************************************************************************/
void USB_HostTaskFn(void *param)
{
    USB_HostEhciTaskFunction(param);
}

/*******************************************************************************/
/*!
 * @brief USB isr function.
 */

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{

    usb_status_t status = kStatus_USB_Success;
    switch (eventCode & 0x0000FFFFU)
    {
    case kUSB_HostEventAttach:
        printf("kUSB_HostEventAttach\r\n");
        status = USB_HostMsdEvent(deviceHandle, configurationHandle, eventCode);
        stopReadingUsb = false;
        usb_attached = true;
        break;

    case kUSB_HostEventNotSupported:
        LOG_INF("Unsupported Device\r\n");

        break;

    case kUSB_HostEventEnumerationDone:
        status = USB_HostMsdEvent(deviceHandle, configurationHandle, eventCode);
        break;

    case kUSB_HostEventDetach:
        printf("kUSB_HostEventDetach\r\n");
        status = USB_HostMsdEvent(deviceHandle, configurationHandle, eventCode);
        stopReadingUsb = true;
        usb_attached = false;
        break;

    case kUSB_HostEventEnumerationFail:
        LOG_INF("enumeration failed\r\n");
        break;

    default:
        break;
    }
    return status;
}

/******************************************************************************/
static void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        // LOG_INF("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    // LOG_INF("host init done\r\n");
};

/******************************************************************************/

void update_progress_bar(uint32_t fileSizeInBytes, size_t bytesRead)
{
    
    printf("PROGRESSING...\r\n");
    // uint32_t percentageComplete = (bytesRead * 100) / fileSizeInBytes ;
    // lv_bar_set_value(bar1, percentageComplete, LV_ANIM_OFF);
    // if (percentageComplete >= 100)
    // {
    //     lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_GREEN));
    //     lv_obj_add_style(bar1, &style_indic, LV_PART_INDICATOR);
    //     lv_label_set_text(update_label, "Remove USB, Rebooting shortly...");
    // }
    // lv_task_handler();
}


void display_read_error(void *displayString)
{
    printf("error ocured \r\n");
    // if (displayString == NULL)
    // {
    //     displayString = "Error during update..."; // Generic error if no message passed.
    // }
    // lv_bar_set_value(bar1, 100, LV_ANIM_OFF);
    // static lv_style_t style_indic;
    // lv_style_init(&style_indic);
    // lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    // lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
    // lv_obj_add_style(bar1, &style_indic, LV_PART_INDICATOR);
    // lv_label_set_text(update_label, displayString);
}


void update_screen_init(void)
{
    updateScreen = lv_obj_create(NULL);

    lv_obj_clean(updateScreen);
    bar1 = lv_bar_create(updateScreen);
    lv_obj_set_size(bar1, 300, 25);
    lv_obj_center(bar1);
    lv_bar_set_value(bar1, 0, LV_ANIM_OFF);
    lv_bar_set_range(bar1, 0, 100);

    update_label = lv_label_create(updateScreen);
    lv_obj_align(update_label, LV_ALIGN_CENTER, 0, -100);
    lv_label_set_text(update_label, "USB update in progress...");
}

static void anim_x_cb(void *var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void *var, int32_t v)
{

    // lv_obj_set_size(var, v, v);

    lv_bar_set_value(guider_ui.screen_bar_1, v, LV_ANIM_OFF);
    lv_bar_set_value(guider_ui.screen_bar_2, v, LV_ANIM_OFF);
    lv_bar_set_value(guider_ui.screen_bar_3, v, LV_ANIM_OFF);
    lv_bar_set_value(guider_ui.screen_bar_4, v, LV_ANIM_OFF);

    lv_arc_set_value(guider_ui.screen_right_btm_arc, v);
    lv_arc_set_value(guider_ui.screen_left_top_arc, v);
    lv_arc_set_value(guider_ui.screen_right_top_arc, v);
    lv_arc_set_value(guider_ui.screen_left_btm_arc, v);

    lv_meter_set_indicator_value(guider_ui.screen_meter_1, guider_ui.screen_meter_1_scale_0_ndline_0, (v * 50));
}

/**
 * Create a playback animation
 */
void lv_example_anim_2(void)
{
    lv_anim_init(&a);
    lv_anim_set_var(&a, &guider_ui);
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

