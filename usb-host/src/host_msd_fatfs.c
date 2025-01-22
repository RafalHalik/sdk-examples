/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016, 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <zephyr/kernel.h>
#include <stdio.h>
#include "usb_host_config.h"
#include "usb_host.h"
#include "usb_host_msd.h"
#include "host_msd_fatfs.h"
#include "ff.h"
#include "diskio.h"
#include "fsl_device_registers.h"
#include "app.h"

 //===========================================
#include <zephyr/init.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/dfu/flash_img.h>

#include <zephyr/sys/reboot.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SLOT1_LABEL slot1_partition
#define SLOT1_SIZE FIXED_PARTITION_SIZE(SLOT1_LABEL)
#define SLOT1_ID FIXED_PARTITION_ID(SLOT1_LABEL)
#define SLOT1_OFFSET FIXED_PARTITION_OFFSET(SLOT1_LABEL)

#define STORAGE_LABEL storage_partition
#define STORAGE_DEV FIXED_PARTITION_DEVICE(STORAGE_LABEL)
#define STORAGE_OFFSET FIXED_PARTITION_OFFSET(STORAGE_LABEL)
#define USBDISK 1

// #define THROUGHPUT_BUFFER_SIZE (32 * 1024) 
#if MSD_FATFS_THROUGHPUT_TEST_ENABLE
#include "fsl_device_registers.h"
// #define THROUGHPUT_BUFFER_SIZE (32 * 1024) /* throughput test buffer */
#define MCU_CORE_CLOCK (120000000)         /* mcu core clock, user need to configure it. */
#endif                                     /* MSD_FATFS_THROUGHPUT_TEST_ENABLE */

typedef enum _usb_update_state
{
    usb_update_mount_drive = 0,
    usb_update_read_file,
    usb_update_write_to_flash,
    usb_update_move_to_next_section,
    usb_check_image,
    usb_update_idle
} usb_update_state;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

 /*!
  * @brief host msd control transfer callback.
  *
  * This function is used as callback function for control transfer .
  *
  * @param param      the host msd fatfs instance pointer.
  * @param data       data buffer pointer.
  * @param data_length data length.
  * @status           transfer result status.
  */
void usb_host_msd_control_callback(void* param, uint8_t* data, uint32_t data_length, usb_status_t status);

/*!
 * @brief display File information.
 */
static void usb_host_msd_fatfs_display_file_info(FILINFO* File_info);

/*!
 * @brief list files and sub-directory in one directory, the function don't check all sub-directories recursively.
 */
static FRESULT usb_host_msd_fatfs_list_directory(const TCHAR* path);



/*******************************************************************************
 * Veethree Fnc
 ******************************************************************************/

 /*!
  * @brief Unblocks/block flash write thread
  *
  * This function sets a flag to either block or unblock the write flash task
  * This is called when a USB drive is attached enumerated and mounted with
  * update File present.
  *
  * @param updating  Flag, if USB drive is set up and ready for reading
  */
extern void start_update_process(bool updating);

/*!
 * @brief changes value on update screens progress bar to indicate amount left
 *
 * Takes the size of the File read and total bytes of data read, calculating a %
 * which is then displayed on screen in the form a LVGL bar object
 *
 * @param file_size_in_bytes size of the File being read
 * @param bytes_read amount of data read from usb so far in bytes
 */
extern void update_progress_bar(uint32_t file_size_in_bytes, size_t bytes_read);


/*!
 * @brief Takes read result from fatfs and writes it to flash
 *
 * After fatfs reads the max amount of bytes it can hold in its buffer this function is called
 * to write the buffered data to flash in chuncks that can be user altered with the #define THROUGHPUT_BUFFER_SIZE
 *
 * @param bytes_left_to_read total data to be written to memory
 */
void write_to_flash(uint32_t bytes_left_to_read);



/*!
 * @brief very basic image validity check
 */
void check_image(void);


/*******************************************************************************
 * Variables
 ******************************************************************************/
 /*! @brief msd class handle array for fatfs */
extern usb_host_class_handle g_UsbFatfsClassHandle;

static struct flash_img_context Flash_ctx;
static usb_update_state Usb_update = usb_update_idle;

static FRESULT Fatfs_code;
static FIL File;
static FILINFO File_info;
static uint8_t driver_number_buffer[3];

usb_host_msd_fatfs_instance_t g_MsdFatfsInstance; /* global msd fatfs instance */
static FATFS fatfs;


#define THROUGHPUT_BUFFER_SIZE (64 * 1024) //(512) //(16 * 1024) //

static uint32_t throughput_buffer[THROUGHPUT_BUFFER_SIZE / 4];
static bool ready_to_read = false;
static bool file_written_to_flash = false;

/* control transfer on-going state. It should set to 1 when start control transfer, it is set to 0 in the callback */
volatile uint8_t control_ing;
/* control transfer callback status */
volatile usb_status_t control_status;

/*******************************************************************************
 * Code
 ******************************************************************************/



void usb_host_msd_control_callback(void* param, uint8_t* data, uint32_t data_length, usb_status_t status)
{
    usb_host_msd_fatfs_instance_t* Msd_fatfs_instance = (usb_host_msd_fatfs_instance_t*)param;

    if (Msd_fatfs_instance->runWaitState == kUSB_HostMsdRunWaitSetInterface) /* set interface finish */
    {
        Msd_fatfs_instance->runWaitState = kUSB_HostMsdRunIdle;
        Msd_fatfs_instance->runState = kUSB_HostMsdRunMassStorageTest;
    }
    control_ing = 0;
    control_status = status;
}


static void usb_host_msd_fatfs_display_file_info(FILINFO* File_info)
{
    char* file_name;
    file_name = File_info->fname;
    /* note: if this File/directory don't have one attribute, '_' replace the attribute letter ('R' - readonly, 'H' -
     * hide, 'S' - system) */
    usb_echo("    %s - %c%c%c - %s - %dBytes - %d-%d-%d %d:%d:%d\r\n\r\n", (File_info->fattrib & AM_DIR) ? "dir" : "fil",
        (File_info->fattrib & AM_RDO) ? 'R' : '_', (File_info->fattrib & AM_HID) ? 'H' : '_',
        (File_info->fattrib & AM_SYS) ? 'S' : '_', file_name, (File_info->fsize),
        (uint32_t)((File_info->fdate >> 9) + 1980) /* year */,
        (uint32_t)((File_info->fdate >> 5) & 0x000Fu) /* month */, (uint32_t)(File_info->fdate & 0x001Fu) /* day */,
        (uint32_t)((File_info->ftime >> 11) & 0x0000001Fu) /* hour */,
        (uint32_t)((File_info->ftime >> 5) & 0x0000003Fu) /* minute */,
        (uint32_t)(File_info->ftime & 0x0000001Fu) /* second */
    );
}

static FRESULT usb_host_msd_fatfs_list_directory(const TCHAR* path)
{
    FRESULT Fatfs_code = FR_OK;
    FILINFO File_info;
    DIR dir;
    uint8_t output_label = 0;

    Fatfs_code = f_opendir(&dir, path);
    if (Fatfs_code)
    {
        return Fatfs_code;
    }
    while (1)
    {
        Fatfs_code = f_readdir(&dir, &File_info);
        if ((Fatfs_code) || (!File_info.fname[0]))
        {
            break;
        }
        output_label = 1;
        usb_host_msd_fatfs_display_file_info(&File_info);
    }
    if (!output_label)
    {
        // usb_echo("\r\n");
    }

    return Fatfs_code;
}

void usb_host_msd_task(void* arg)
{
    usb_status_t status;
    usb_host_msd_fatfs_instance_t* Msd_fatfs_instance = (usb_host_msd_fatfs_instance_t*)arg;
    // usb_echo("usb_host_msd_task \r\n");
    if (Msd_fatfs_instance->deviceState != Msd_fatfs_instance->prevDeviceState)
    {
        Msd_fatfs_instance->prevDeviceState = Msd_fatfs_instance->deviceState;
        switch (Msd_fatfs_instance->deviceState)
        {
        case kStatus_DEV_Idle:    
         usb_echo("kStatus_DEV_Idle \r\n");      
            break;

        case kStatus_DEV_Attached: /* deivce is attached and numeration is done */
         usb_echo("kStatus_DEV_Attached \r\n");
            status = usb_host_msd_init(Msd_fatfs_instance->Device_handle,
                &Msd_fatfs_instance->classHandle); /* msd class initialization */
            g_UsbFatfsClassHandle = Msd_fatfs_instance->classHandle;
            if (status != kStatus_USB_Success)
            {
                usb_echo("usb host msd init fail\r\n");
                return;
            }
            Msd_fatfs_instance->runState = kUSB_HostMsdRunSetInterface;
            break;

        case kStatus_DEV_Detached: /* device is detached */
          usb_echo("kStatus_DEV_Detached \r\n");
            Msd_fatfs_instance->deviceState = kStatus_DEV_Idle;
            Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;
            usb_host_msd_deinit(Msd_fatfs_instance->Device_handle,
                Msd_fatfs_instance->classHandle); /* msd class de-initialization */
            Msd_fatfs_instance->classHandle = NULL;
            //g_MsdFatfsInstance.deviceState = kStatus_DEV_Detached;
            start_update_process(false);
            usb_echo("mass storage device detached\r\n");
            break;

        default:
            break;
        }
    }

    /* run state */
    switch (Msd_fatfs_instance->runState)
    {
    case kUSB_HostMsdRunIdle:
        break;

    case kUSB_HostMsdRunSetInterface: /* set msd interface */
    usb_echo("kUSB_HostMsdRunSetInterface \r\n");
        Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;
        Msd_fatfs_instance->runWaitState = kUSB_HostMsdRunWaitSetInterface;
        status = usb_host_msd_set_interface(Msd_fatfs_instance->classHandle, Msd_fatfs_instance->interfaceHandle, 0,
            usb_host_msd_control_callback, Msd_fatfs_instance);
        if (status != kStatus_USB_Success)
        {
            // usb_echo("set interface fail\r\n");
        }
        break;

    case kUSB_HostMsdRunMassStorageTest: /* set interface succeed */
    usb_echo("kUSB_HostMsdRunMassStorageTest \r\n");
        Usb_update = usb_update_mount_drive;
        ready_to_read = true;
        start_update_process(true);
        Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;
        //USB_MountStorageDeviceFatfs(Msd_fatfs_instance);
        // Msd_fatfs_instance->runState = kUSB_HostMsdRunUpdate;
        break;
    case kUSB_HostMsdRunUpdate:
    usb_echo("kUSB_HostMsdRunUpdate \r\n");
        start_update_process(true);
        usb_echo("USB update case... \r\n");
        Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;
        break;
    default:
        break;
    }
}



void usb_update_task(void)
{
    static uint32_t bytes_left_to_read = 0;
    static uint32_t result_size = 1;
    static uint8_t file_read_counter = 0;
    static size_t result_size_total = 0;

    static uint8_t debug_number_of_writes = 0;

    switch (Usb_update)
    {
    case usb_update_mount_drive:       
        // Reinitalize varibles incase somethign went wrong and update had to be attempted again...
        bytes_left_to_read = 0;
        result_size = 1;
        file_read_counter = 0;
        result_size_total = 0;
        debug_number_of_writes = 0;

        sprintf((char*)&driver_number_buffer[0], "%c:", USBDISK + '0');
        Fatfs_code = f_mount(&fatfs, (char const*)&driver_number_buffer[0], 0);
        if (Fatfs_code)
        {
            return;
        }

        Fatfs_code = usb_host_msd_fatfs_list_directory((char const*)&driver_number_buffer[0]);
        if (Fatfs_code)
        {
            return;
        }

        Fatfs_code = f_open(&File, _T("1:/zephyr~1.bin"), FA_READ | FA_OPEN_EXISTING);
        if (Fatfs_code)
        {
            usb_echo("error File no able to be opended\r\n");
        }
        else
        {
            Fatfs_code = f_stat(_T("1:/zephyr~1.bin"), &File_info);
            if (Fatfs_code)
            {
                return;
            }
        }
        flash_img_init_id(&Flash_ctx, SLOT1_ID);
        flash_area_erase(Flash_ctx.flash_area, SLOT1_ID, Flash_ctx.flash_area->fa_size);

        bytes_left_to_read = File_info.fsize;
        Usb_update = usb_update_read_file;
        break;
    case usb_update_read_file:
            Fatfs_code = f_read(&File, throughput_buffer, THROUGHPUT_BUFFER_SIZE, &result_size);
            if (Fatfs_code)
            {
                usb_echo("read error %d \r\n", Fatfs_code);                        
                update_failed_clean_up("Error Reading! try again...");
                return;
            }           

            bytes_left_to_read -= result_size;
            Usb_update = usb_update_write_to_flash;
        break;
    case usb_update_write_to_flash:       
        write_to_flash(bytes_left_to_read);
        debug_number_of_writes++;
        result_size_total += result_size;
        update_progress_bar(File_info.fsize, result_size_total);

        Usb_update = usb_update_move_to_next_section;
        break;
    case usb_update_move_to_next_section:     
        Fatfs_code = f_lseek(&File, result_size_total);
        if (Fatfs_code)
        {
            usb_echo("seek error\r\n");
            Usb_update = usb_update_idle;
            f_close(&File);
            return;
        }
        if ((flash_img_bytes_written(&Flash_ctx) >= File_info.fsize) || (bytes_left_to_read == 0))
        {
            Usb_update = usb_check_image;
        }
        else
        {
            Usb_update = usb_update_read_file;
        }
        break;
    case usb_check_image:
        usb_echo("usb_check_image \r\n");
        check_image();
        Usb_update = usb_update_idle;
        ready_to_read = false;
        break;
    case usb_update_idle:
        // wait
        if (ready_to_read == true)
        {
            Usb_update = usb_update_mount_drive;
        }
        else
        {
            k_msleep(10);
            Usb_update = usb_update_idle;
        }        
        break;
    default:
        usb_echo("Unknown state... \r\n");
        break;
    }

}

void update_failed_clean_up(void* display_string)
{
    // display_read_error(display_string);   
    ready_to_read = false;
    file_written_to_flash = false;
    Usb_update = usb_update_idle; 
    f_close(&File);
}


void check_image(void)
{
    struct mcuboot_img_header header;
    boot_read_bank_header(FIXED_PARTITION_ID(slot1_partition), &header, sizeof(header));
    usb_echo("mcuboot_version: %d \r\n", header.mcuboot_version);
    usb_echo("image_size: %d \r\n", header.h.v1.image_size);
    usb_echo("img version: %d.%d \r\n", header.h.v1.sem_ver.major, header.h.v1.sem_ver.minor);
    usb_echo("Image check... \r\n");
    
    if(((header.h.v1.sem_ver.major == 0) && ( header.h.v1.sem_ver.minor == 0)) || (header.h.v1.image_size == 0))
    {
        usb_echo("Image invalid... \r\n");
        update_failed_clean_up("Image invalid...");
        return;
    }

    if (boot_request_upgrade(BOOT_UPGRADE_PERMANENT))
    {
        usb_echo("Upgrade request FAIL \r\n");
        return;
    }
   
    file_written_to_flash = true;
}

void write_to_flash(uint32_t bytes_left_to_read)
{  
    if(Usb_update == usb_update_write_to_flash)
    {
        int ret = 0;

        bool flush = false;
        if (bytes_left_to_read == 0)
        {
            // whole File read now force flush/write of buffered data before reboot
            flush = true;        
        }
        // usb_echo("bytes_left_to_read %d \r\n",bytes_left_to_read);
        ret = flash_img_buffered_write(&Flash_ctx, (uint8_t *)throughput_buffer, THROUGHPUT_BUFFER_SIZE, flush);
       
        if (ret)
        {
            // usb_echo("bytes_left_to_read %d \r\n",bytes_left_to_read);
            
            usb_echo("flash write failed \r\n");
            return;
        }
    }
}

usb_status_t usb_host_msd_event(usb_device_handle Device_handle,
    usb_host_configuration_handle Configuration_handle,
    uint32_t event_code)
{
    usb_status_t status = kStatus_USB_Success;
    usb_host_configuration_t* configuration;
    uint8_t interface_index;
    usb_host_interface_t* interface;
    uint32_t info_value = 0U;
    uint8_t id;

    switch (event_code)
    {
    case kUSB_HostEventAttach:
        /* judge whether is Configuration_handle supported */
        configuration = (usb_host_configuration_t*)Configuration_handle;
        for (interface_index = 0; interface_index < configuration->interfaceCount; ++interface_index)
        {
            interface = &configuration->interfaceList[interface_index];
            id = interface->interfaceDesc->bInterfaceClass;
            if (id != USB_HOST_MSD_CLASS_CODE)
            {
                continue;
            }
            id = interface->interfaceDesc->bInterfaceSubClass;
            if ((id != USB_HOST_MSD_SUBCLASS_CODE_UFI) && (id != USB_HOST_MSD_SUBCLASS_CODE_SCSI))
            {
                continue;
            }
            id = interface->interfaceDesc->bInterfaceProtocol;
            if (id != USB_HOST_MSD_PROTOCOL_BULK)
            {
                continue;
            }
            else
            {
                if (g_MsdFatfsInstance.deviceState == kStatus_DEV_Idle)
                {
                    /* the interface is supported by the application */
                    g_MsdFatfsInstance.Device_handle = Device_handle;
                    g_MsdFatfsInstance.interfaceHandle = interface;
                    g_MsdFatfsInstance.configHandle = Configuration_handle;
                    return kStatus_USB_Success;
                }
                else
                {
                    continue;
                }
            }
        }
        status = kStatus_USB_NotSupported;
        break;

    case kUSB_HostEventNotSupported:
        break;

    case kUSB_HostEventEnumerationDone:
        if (g_MsdFatfsInstance.configHandle == Configuration_handle)
        {
            if ((g_MsdFatfsInstance.Device_handle != NULL) && (g_MsdFatfsInstance.interfaceHandle != NULL))
            {
                /* the device enumeration is done */
                if (g_MsdFatfsInstance.deviceState == kStatus_DEV_Idle)
                {
                    g_MsdFatfsInstance.deviceState = kStatus_DEV_Attached;

                    usb_host_helper_get_peripheral_information(Device_handle, kUSB_HostGetDevicePID, &info_value);
                    // usb_echo("mass storage device attached:pid=0x%x", info_value);
                    usb_host_helper_get_peripheral_information(Device_handle, kUSB_HostGetDeviceVID, &info_value);
                    // usb_echo("vid=0x%x ", info_value);
                    usb_host_helper_get_peripheral_information(Device_handle, kUSB_HostGetDeviceAddress, &info_value);
                    // usb_echo("address=%d\r\n", info_value);
                }
                else
                {
                    // usb_echo("not idle msd instance\r\n");
                    status = kStatus_USB_Error;
                }
            }
        }
        break;

    case kUSB_HostEventDetach:
        if (g_MsdFatfsInstance.configHandle == Configuration_handle)
        {
            /* the device is detached */
            g_UsbFatfsClassHandle = NULL;
            g_MsdFatfsInstance.configHandle = NULL;
            if (g_MsdFatfsInstance.deviceState != kStatus_DEV_Idle)
            {
                usb_echo("status DETACHED \r\n");
                if (file_written_to_flash == true)
                {
                    NVIC_SystemReset(); //reboot, restart, powercycle, load new image
                }
                g_MsdFatfsInstance.deviceState = kStatus_DEV_Detached;
            }
        }

        break;

    default:
        break;
    }
    return status;
}
