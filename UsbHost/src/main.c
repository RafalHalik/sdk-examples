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
 * @param deviceHandle        device handle.
 * @param configurationHandle attached device's configuration descriptor information.
 * @param eventCode           callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);



static void USB_HostApplicationInit(void);


/*! @brief USB host msd fatfs instance global variable */
extern usb_host_msd_fatfs_instance_t g_MsdFatfsInstance;
usb_host_handle g_HostHandle;

/* Run USB stack on a separate thread */


/******************************************************************************/
/* Functions */
/******************************************************************************/

void start_update_process(bool updating);
void stop_read_usb(void);

static bool stopReadingUsb = false;

char count_str[11] = {0};
bool updateInProgress = false;

#define SEM_COUNT_INITAL 0
#define SEM_COUNT_MAX 1


/******************************************************************************/
void start_update_process(bool updating)
{
    updateInProgress = updating;
}

void dynamic_usb_isr(const void *param)
{
    USB_HostEhciIsrFunction(g_HostHandle);
}



void update_progress_bar(uint32_t fileSizeInBytes, size_t bytesRead)
{
    uint32_t percentageComplete = 0;
    percentageComplete = (bytesRead * 100) / fileSizeInBytes;
    char buffer[100];

    snprintf(buffer, sizeof(buffer), 
        "\rUpdate in progress... %d %%   |   Bytes written: %d / %d", percentageComplete, bytesRead, fileSizeInBytes);

    printf(buffer);
   
}

/******************************************************************************/

struct device *display_dev;
int main(void)
{
    USB_HostApplicationInit();
    const struct device *pointer;
    

    while (1)
    {        
        USB_HostTaskFn(g_HostHandle);
        USB_HostMsdTask(&g_MsdFatfsInstance);      
        usb_update_task();
        k_msleep(1);
    }

    return 0;
}



/*******************************************************************************/
void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0) //480000000UL
    {
         CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000); //24000000
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000); //24000000

    }
    else
    {
          CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 24000000); //24000000
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 24000000); //24000000

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
        LOG_INF("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    LOG_INF("host init done\r\n");
};




