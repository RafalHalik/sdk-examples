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
#include <zephyr/irq.h>

#include "board.h"
#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "fsl_common.h"


#include "usb_phy.h"
#include "usb_host_msd.h"
#include "host_msd_fatfs.h"
#include "app.h"

#include "zephyr/dfu/mcuboot.h"
#include "/workdir/modules/hal/nxp/mcux/mcux-sdk/devices/MIMXRT1166/MIMXRT1166_cm7_features.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

bool usb_attached = false;

static void anim_x_cb(void * var, int32_t v);
void lv_example_anim_2(void);
static void anim_size_cb(void * var, int32_t v);


#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)


static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param Device_handle        device handle.
 * @param Configuration_handle attached device's configuration descriptor information.
 * @param event_code           callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t usb_host_event(usb_device_handle Device_handle,
                                  usb_host_configuration_handle Configuration_handle,
                                  uint32_t event_code);



static void usb_host_application_init(void);


/*! @brief USB host msd fatfs instance global variable */
extern usb_host_msd_fatfs_instance_t g_MsdFatfsInstance;
usb_host_handle g_HostHandle;

/* Run USB stack on a separate thread */


/******************************************************************************/
/* Functions */
/******************************************************************************/

void start_update_process(bool updating);
void stop_read_usb(void);

static bool stop_reading_usb = false;

char count_str[11] = {0};
bool update_in_progress = false;

#define SEM_COUNT_INITAL 0
#define SEM_COUNT_MAX 1


/******************************************************************************/
void start_update_process(bool updating)
{
    update_in_progress = updating;
}

void dynamic_usb_isr(const void *param)
{
    usb_host_ehci_isr_function(g_HostHandle);
}



void update_progress_bar(uint32_t file_size_in_bytes, size_t bytes_read)
{
    uint32_t percentage_complete = 0;
    percentage_complete = (bytes_read * 100) / file_size_in_bytes;
    char buffer[100];

    snprintf(buffer, sizeof(buffer), 
        "\rUpdate in progress... %d %%   |   Bytes written: %d / %d", percentage_complete, bytes_read, file_size_in_bytes);

    printf(buffer);
   
}

/******************************************************************************/

struct device *display_dev;
int main(void)
{
    usb_host_application_init();
    const struct device *pointer;
    

    while (1)
    {        
        usb_host_task_fn(g_HostHandle);
        USB_HostMsdTask(&g_MsdFatfsInstance);      
        usb_update_task();
        k_msleep(1);
    }

    return 0;
}



/*******************************************************************************/
void usb_host_clock_init(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0) //480000000UL
    {
        clock_enable_usbhs0_phy_pll_clock(kCLOCK_Usbphy480M, 24000000); //24000000
        clock_enable_usbhs0_clock(kCLOCK_Usb480M, 24000000); //24000000

    }
    else
    {
        clock_enable_usbhs0_phy_pll_clock(kCLOCK_Usbphy480M, 24000000); //24000000
        clock_enable_usbhs0_clock(kCLOCK_Usb480M, 24000000); //24000000

    }                                                     
    usb_status_t stat = usb_ehci_phy_init(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
   usb_echo("stat %d", stat);
}

void usb_host_isr_enable(void)
{
    uint8_t irq_number;

    uint8_t usb_host_ehci_irq[] = USBHS_IRQS;
    irq_number = usb_host_ehci_irq[CONTROLLER_ID - kUSB_ControllerEhci0];

    irq_connect_dynamic(136, USB_HOST_INTERRUPT_PRIORITY, dynamic_usb_isr, NULL, 0);
    irq_enable(136);
}

/*******************************************************************************/
void usb_host_task_fn(void *param)
{
    usb_host_ehci_task_function(param);
}

/*******************************************************************************/
/*!
 * @brief USB isr function.
 */

static usb_status_t usb_host_event(usb_device_handle Device_handle,
                                  usb_host_configuration_handle Configuration_handle,
                                  uint32_t event_code)
{
     
    usb_status_t status = kStatus_USB_Success;
    switch (event_code & 0x0000FFFFU)
    {
    case kUSB_HostEventAttach:
        printf("kUSB_HostEventAttach\r\n");
        status = usb_host_msd_event(Device_handle, Configuration_handle, event_code);
        stop_reading_usb = false;
        usb_attached = true;
        break;

    case kUSB_HostEventNotSupported:
        LOG_INF("Unsupported Device\r\n");

        break;

    case kUSB_HostEventEnumerationDone:
        status = usb_host_msd_event(Device_handle, Configuration_handle, event_code);
        break;

    case kUSB_HostEventDetach:
        printf("kUSB_HostEventDetach\r\n");
        status = usb_host_msd_event(Device_handle, Configuration_handle, event_code);
        stop_reading_usb = true;
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
static void usb_host_application_init(void)
{
    usb_status_t status = kStatus_USB_Success;

    usb_host_clock_init();

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, usb_host_event);
    if (status != kStatus_USB_Success)
    {
        LOG_INF("host init error\r\n");
        return;
    }
    usb_host_isr_enable();

    LOG_INF("host init done\r\n");
};




