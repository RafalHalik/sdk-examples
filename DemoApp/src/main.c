/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "board.h"
#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "fsl_common.h"

#include "../generated/gui_guider.h"
#include "../generated/events_init.h"

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


extern usb_host_msd_fatfs_instance_t g_MsdFatfsInstance;
usb_host_handle g_HostHandle;

struct k_thread usb_thread_data;

lv_anim_t a;

static lv_style_t style_indic;
static lv_obj_t *mainScreen;


static bool stopReadingUsb = false;
bool updateInProgress = false;

bool usb_attached = false;
lv_ui guider_ui;

struct device *display_dev;

void start_update_process(bool updating);

static void anim_x_cb(void *var, int32_t v);
void lv_example_anim_2(void);
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

int main(void)
{
    char buffer[100];
    uint64_t start_time, end_time = 0;
    uint32_t fps = 0;
   
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    mainScreen = lv_scr_act();

    update_screen_init(&guider_ui);

    
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
              
        if (updateInProgress)
        {
            usb_update_task(&guider_ui);
            if (lv_scr_act() != guider_ui.updateScreen)
            {
                lv_scr_load(guider_ui.updateScreen);
                lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
                lv_obj_add_style(guider_ui.update_srn_bar_1, &style_indic, LV_PART_INDICATOR);
                lv_label_set_text(guider_ui.update_label, "USB update in progress...");
                lv_bar_set_value(guider_ui.update_srn_bar_1, 0, LV_ANIM_OFF);
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
       
        fps++;
        end_time = k_uptime_get();
        if ((end_time - start_time) >= 1000)
        {           
            fps = 0;
            start_time = k_uptime_get();
        }        
       
        k_msleep(30);
    }

    return 0;
}


void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0) 
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000);          
    }
    else
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000); 
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000);         
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

void USB_HostTaskFn(void *param)
{
    USB_HostEhciTaskFunction(param);
}


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

static void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {       
        return;
    }
    USB_HostIsrEnable();   
};


void update_progress_bar(lv_ui *ui, uint32_t fileSizeInBytes, size_t bytesRead)
{
    
    printf("PROGRESSING...\r\n");
    uint32_t percentageComplete = (bytesRead * 100) / fileSizeInBytes ;
    lv_bar_set_value(ui->update_srn_bar_1, percentageComplete, LV_ANIM_OFF);
    if (percentageComplete >= 100)
    {
        lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_GREEN));
        lv_obj_add_style(ui->update_srn_bar_1, &style_indic, LV_PART_INDICATOR);
        lv_label_set_text(ui->update_label, "Remove USB, Rebooting shortly...");
    }
    lv_task_handler();
}


void display_read_error(lv_ui *ui, void *displayString)
{
    printf("error ocured \r\n");
    if (displayString == NULL)
    {
        displayString = "Error during update..."; // Generic error if no message passed.
    }
    lv_bar_set_value(ui->update_srn_bar_1, 100, LV_ANIM_OFF);
    static lv_style_t style_indic;
    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
    lv_obj_add_style(ui->update_srn_bar_1, &style_indic, LV_PART_INDICATOR);
    lv_label_set_text(ui->update_label, displayString);
}


void update_screen_init(lv_ui *ui)
{
    ui->updateScreen = lv_obj_create(NULL);

    lv_obj_set_size(ui->updateScreen, 720, 1280);
    lv_obj_clean(ui->updateScreen);
    lv_obj_set_pos(ui->updateScreen, 0, 0);
    ui->update_srn_bar_1 = lv_bar_create(ui->updateScreen);
    lv_obj_set_size(ui->update_srn_bar_1, 300, 25);
    lv_obj_center(ui->update_srn_bar_1);
    lv_bar_set_value(ui->update_srn_bar_1, 0, LV_ANIM_OFF);
    lv_bar_set_range(ui->update_srn_bar_1, 0, 100);

    ui->update_label = lv_label_create(ui->updateScreen);
    lv_obj_align(ui->update_label, LV_ALIGN_CENTER, 0, -100);
    lv_label_set_text(ui->update_label, "USB update in progress...");
}

static void anim_x_cb(void *var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void *var, int32_t v)
{  
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

