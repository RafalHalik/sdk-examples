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
#include "gui_guider.h"

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

#if MSD_FATFS_THROUGHPUT_TEST_ENABLE
#include "fsl_device_registers.h"
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

static uint8_t test_buffer[(FF_MAX_SS > 256) ? FF_MAX_SS : 256]; 

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


static void usb_host_msd_fatfs_test_done(void);

static void usb_host_msd_fatfs_test(usb_host_msd_fatfs_instance_t *Msd_fatfs_instance);

 /*!
  * @brief host msd control transfer callback.
  *
  * This function is used as callback function for control transfer .
  *
  * @param param      the host msd Fatfs instance pointer.
  * @param data       data buffer pointer.
  * @param data_length data length.
  * @status           transfer result status.
  */
void USB_HostMsdControlCallback(void* param, uint8_t* data, uint32_t data_length, usb_status_t status);

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
extern void update_progress_bar(lv_ui *Ui, uint32_t file_size_in_bytes, size_t bytes_read);


/*!
 * @brief Takes read result from Fatfs and writes it to flash
 *
 * After Fatfs reads the max amount of bytes it can hold in its buffer this function is called
 * to write the buffered data to flash in chuncks that can be user altered with the #define THROUGHPUT_BUFFER_SIZE
 *
 * @param bytes_left_to_read total data to be written to memory
 */
void write_to_flash(uint32_t bytes_left_to_read);


extern void display_read_error(lv_ui *Ui, void *display_string);

/*!
 * @brief very basic image validity check
 */
void check_image(lv_ui *Ui);


/*******************************************************************************
 * Variables
 ******************************************************************************/

static struct flash_img_context Flash_ctx;
static usb_update_state Usb_update = usb_update_idle;
static FRESULT Fatfs_code;
static FIL File;
static FILINFO File_info;

static bool ready_to_read = false;
static bool file_written_to_flash = false;


static uint8_t driver_number_buffer[3];

 /*! @brief msd class handle array for Fatfs */
extern usb_host_class_handle g_UsbFatfsClassHandle;

usb_host_msd_fatfs_instance_t g_MsdFatfsInstance; /* global msd Fatfs instance */
static FATFS Fatfs;
/* control transfer on-going state. It should set to 1 when start control transfer, it is set to 0 in the callback */
volatile uint8_t control_ing;
/* control transfer callback status */
volatile usb_status_t Control_status;

#define THROUGHPUT_BUFFER_SIZE (64 * 1024) //(512) //(16 * 1024) //(64 * 1024)
static uint32_t throughput_buffer[THROUGHPUT_BUFFER_SIZE / 4];


/*******************************************************************************
 * Code
 ******************************************************************************/



void USB_HostMsdControlCallback(void* param, uint8_t* data, uint32_t data_length, usb_status_t status)
{
    usb_host_msd_fatfs_instance_t* Msd_fatfs_instance = (usb_host_msd_fatfs_instance_t*)param;

    if (Msd_fatfs_instance->runWaitState == kUSB_HostMsdRunWaitSetInterface) /* set Interface finish */
    {
        Msd_fatfs_instance->runWaitState = kUSB_HostMsdRunIdle;
        Msd_fatfs_instance->runState = kUSB_HostMsdRunMassStorageTest;
    }
    control_ing = 0;
    Control_status = status;
}


static void usb_host_msd_fatfs_display_file_info(FILINFO* File_info)
{
    char* file_name;
    file_name = File_info->fname;
    /* note: if this File/directory don't have one attribute, '_' replace the attribute letter ('R' - readonly, 'H' -
     * hide, 'S' - system) */
    usb_echo("    %s - %c%c%c - %s - %dBytes - %d-%d-%d %d:%d:%d\r\n", (File_info->fattrib & AM_DIR) ? "Dir" : "fil",
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
    DIR Dir;
    uint8_t output_label = 0;

    Fatfs_code = f_opendir(&Dir, path);
    if (Fatfs_code)
    {
        return Fatfs_code;
    }
    while (1)
    {
        Fatfs_code = f_readdir(&Dir, &File_info);
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
            status = USB_HostMsdInit(Msd_fatfs_instance->deviceHandle,
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
            USB_HostMsdDeinit(Msd_fatfs_instance->deviceHandle,
                Msd_fatfs_instance->classHandle); /* msd class de-initialization */
            Msd_fatfs_instance->classHandle = NULL;          
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

    case kUSB_HostMsdRunSetInterface: /* set msd Interface */
    usb_echo("kUSB_HostMsdRunSetInterface \r\n");
        Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;
        Msd_fatfs_instance->runWaitState = kUSB_HostMsdRunWaitSetInterface;
        status = USB_HostMsdSetInterface(Msd_fatfs_instance->classHandle, Msd_fatfs_instance->interfaceHandle, 0,
            USB_HostMsdControlCallback, Msd_fatfs_instance);
        if (status != kStatus_USB_Success)
        {
            // usb_echo("set Interface fail\r\n");
        }
        break;

    case kUSB_HostMsdRunMassStorageTest: /* set Interface succeed */
    usb_echo("kUSB_HostMsdRunMassStorageTest \r\n");
        Usb_update = usb_update_mount_drive;
        ready_to_read = true;
        start_update_process(true);
        Msd_fatfs_instance->runState = kUSB_HostMsdRunIdle;      
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



void usb_update_task(lv_ui *Ui)
{
    static uint32_t bytes_left_to_read = 0;
    static uint32_t result_size = 1;
    static uint8_t fileReadCounter = 0;
    static size_t resultSizeTotal = 0;

    static uint8_t DEBUG_number_of_writes = 0;

    switch (Usb_update)
    {
    case usb_update_mount_drive:       
        // Reinitalize varibles incase somethign went wrong and update had to be attempted again...
        bytes_left_to_read = 0;
        result_size = 1;
        fileReadCounter = 0;
        resultSizeTotal = 0;
        DEBUG_number_of_writes = 0;

        sprintf((char*)&driver_number_buffer[0], "%c:", USBDISK + '0');
        Fatfs_code = f_mount(&Fatfs, (char const*)&driver_number_buffer[0], 0);
        if (Fatfs_code)
        {
             usb_echo("error mounting %d\r\n", Fatfs_code);
            return;
        }

        Fatfs_code = usb_host_msd_fatfs_list_directory((char const*)&driver_number_buffer[0]);
        if (Fatfs_code)
        {
             usb_echo("error reading drive %d\r\n", Fatfs_code);
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
        DEBUG_number_of_writes++;
        resultSizeTotal += result_size;
        update_progress_bar(Ui, File_info.fsize, resultSizeTotal);

        Usb_update = usb_update_move_to_next_section;
        break;
    case usb_update_move_to_next_section:     
        Fatfs_code = f_lseek(&File, resultSizeTotal);
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
        check_image(Ui);
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
            k_msleep(5);
            Usb_update = usb_update_idle;
        }        
        break;
    default:
        usb_echo("Unknown state... \r\n");
        break;
    }

}

void update_failed_clean_up(lv_ui *Ui, void* display_string)
{
    display_read_error(Ui, display_string);   
    ready_to_read = false;
    file_written_to_flash = false;
    Usb_update = usb_update_idle; 
    f_close(&File);
}


void check_image(lv_ui *Ui)
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
        update_failed_clean_up(Ui, "Image invalid...");
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
        usb_echo("bytes_left_to_read %d \r\n",bytes_left_to_read);
        ret = flash_img_buffered_write(&Flash_ctx, (uint8_t *)throughput_buffer, THROUGHPUT_BUFFER_SIZE, flush);

        if (ret)
        {
            usb_echo("bytes_left_to_read %d \r\n",bytes_left_to_read);
            
            usb_echo("flash write failed \r\n");
            return;
        }
    }
}

usb_status_t USB_HostMsdEvent(usb_device_handle deviceHandle,
    usb_host_configuration_handle Configuration_handle,
    uint32_t event_code)
{
    usb_status_t status = kStatus_USB_Success;
    usb_host_configuration_t* Configuration;
    usb_host_interface_t* Interface;
    
    uint8_t interface_index;
    uint32_t info_value = 0U;
    uint8_t id;

    switch (event_code)
    {
    case kUSB_HostEventAttach:
        /* judge whether is Configuration_handle supported */
        Configuration = (usb_host_configuration_t*)Configuration_handle;
        for (interface_index = 0; interface_index < Configuration->interfaceCount; ++interface_index)
        {
            Interface = &Configuration->interfaceList[interface_index];
            id = Interface->interfaceDesc->bInterfaceClass;
            if (id != USB_HOST_MSD_CLASS_CODE)
            {
                continue;
            }
            id = Interface->interfaceDesc->bInterfaceSubClass;
            if ((id != USB_HOST_MSD_SUBCLASS_CODE_UFI) && (id != USB_HOST_MSD_SUBCLASS_CODE_SCSI))
            {
                continue;
            }
            id = Interface->interfaceDesc->bInterfaceProtocol;
            if (id != USB_HOST_MSD_PROTOCOL_BULK)
            {
                continue;
            }
            else
            {
                if (g_MsdFatfsInstance.deviceState == kStatus_DEV_Idle)
                {
                    /* the Interface is supported by the application */
                    g_MsdFatfsInstance.deviceHandle = deviceHandle;
                    g_MsdFatfsInstance.interfaceHandle = Interface;
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
            if ((g_MsdFatfsInstance.deviceHandle != NULL) && (g_MsdFatfsInstance.interfaceHandle != NULL))
            {
                /* the device enumeration is done */
                if (g_MsdFatfsInstance.deviceState == kStatus_DEV_Idle)
                {
                    g_MsdFatfsInstance.deviceState = kStatus_DEV_Attached;
                    
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDevicePID, &info_value);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceVID, &info_value);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceAddress, &info_value);
                }   
                else
                {
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
                Usb_update = usb_update_idle;
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



static void usb_host_msd_fatfs_test(usb_host_msd_fatfs_instance_t *Msd_fatfs_instance)
{
    FRESULT Fatfs_code;
    FATFS *fs;
    FIL File;
    FILINFO File_info;

    uint32_t free_cluster_number;
    uint32_t index;
    uint32_t result_size;
    char *test_string;
    uint8_t driver_number_buffer[3];

    /* time delay */
    for (free_cluster_number = 0; free_cluster_number < 10000; ++free_cluster_number)
    {
        __NOP();
    }

    usb_echo("............................Fatfs test.....................\r\n");

    usb_echo("Fatfs mount as logiacal driver %d......", USBDISK);
    sprintf((char *)&driver_number_buffer[0], "%c:", USBDISK + '0');
    Fatfs_code = f_mount(&Fatfs, (char const *)&driver_number_buffer[0], 0);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");

#if (FF_FS_RPATH >= 2)
    Fatfs_code = f_chdrive((char const *)&driver_number_buffer[0]);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
#endif

#if FF_USE_MKFS
    MKFS_PARM Format_options = {FM_SFD | FM_ANY, 0, 0, 0, 0};
    usb_echo("test f_mkfs......");
    Fatfs_code = f_mkfs((char const *)&driver_number_buffer[0], &Format_options, test_buffer, FF_MAX_SS);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
#endif /* FF_USE_MKFS */

    usb_echo("test f_getfree:\r\n");
    Fatfs_code = f_getfree((char const *)&driver_number_buffer[0], (uint32_t *)&free_cluster_number, &fs);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    if (fs->fs_type == FS_FAT12)
    {
        usb_echo("    FAT type = FAT12\r\n");
    }
    else if (fs->fs_type == FS_FAT16)
    {
        usb_echo("    FAT type = FAT16\r\n");
    }
    else
    {
        usb_echo("    FAT type = FAT32\r\n");
    }
    usb_echo("    bytes per cluster = %d; number of clusters=%lu \r\n", fs->csize * 512, fs->n_fatent - 2);
    usb_echo("    The free size: %dKB, the total size:%dKB\r\n", (free_cluster_number * (fs->csize) / 2),
             ((fs->n_fatent - 2) * (fs->csize) / 2));

    usb_echo("directory operation:\r\n");
    usb_echo("list root directory:\r\n");
    Fatfs_code = usb_host_msd_fatfs_list_directory((char const *)&driver_number_buffer[0]);
    if (Fatfs_code)
    {
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("create directory \"dir_1\"......");
    Fatfs_code = f_mkdir(_T("1:/dir_1"));
    if (Fatfs_code)
    {
        if (Fatfs_code == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            usb_host_msd_fatfs_test_done();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("create directory \"dir_2\"......");
    Fatfs_code = f_mkdir(_T("1:/dir_2"));
    if (Fatfs_code)
    {
        if (Fatfs_code == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            usb_host_msd_fatfs_test_done();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("create sub directory \"dir_2/sub_1\"......");
    Fatfs_code = f_mkdir(_T("1:/dir_1/sub_1"));
    if (Fatfs_code)
    {
        if (Fatfs_code == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            usb_host_msd_fatfs_test_done();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("list root directory:\r\n");
    Fatfs_code = usb_host_msd_fatfs_list_directory(_T("1:"));
    if (Fatfs_code)
    {
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("list directory \"dir_1\":\r\n");
    Fatfs_code = usb_host_msd_fatfs_list_directory(_T("1:/dir_1"));
    if (Fatfs_code)
    {
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("rename directory \"dir_1/sub_1\" to \"dir_1/sub_2\"......");
    Fatfs_code = f_rename(_T("1:/dir_1/sub_1"), _T("1:/dir_1/sub_2"));
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("delete directory \"dir_1/sub_2\"......");
    Fatfs_code = f_unlink(_T("1:/dir_1/sub_2"));
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");

#if (FF_FS_RPATH >= 2)
    usb_echo("get current directory......");
    Fatfs_code = f_getcwd((TCHAR *)&test_buffer[0], 256);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("%s\r\n", test_buffer);
    usb_echo("change current directory to \"dir_1\"......");
    Fatfs_code = f_chdir(_T("dir_1"));
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("list current directory:\r\n");
    Fatfs_code = usb_host_msd_fatfs_list_directory(_T("."));
    if (Fatfs_code)
    {
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("get current directory......");
    Fatfs_code = f_getcwd((TCHAR *)&test_buffer[0], 256);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("%s\r\n", test_buffer);
#endif

    usb_echo("get directory \"dir_1\" information:\r\n");
    Fatfs_code = f_stat(_T("1:/dir_1"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_host_msd_fatfs_display_file_info(&File_info);
    usb_echo("change \"dir_1\" timestamp to 2015.10.1, 12:30:0......");
    File_info.fdate = ((2015 - 1980) << 9 | 10 << 5 | 1); /* 2015.10.1 */
    File_info.ftime = (12 << 11 | 30 << 5);               /* 12:30:00 */
    Fatfs_code      = f_utime(_T("1:/dir_1"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get directory \"dir_1\" information:\r\n");
    Fatfs_code = f_stat(_T("1:/dir_1"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_host_msd_fatfs_display_file_info(&File_info);

    usb_echo("File operation:\r\n");
    usb_echo("create File \"f_1.dat\"......");
    Fatfs_code = f_open(&File, _T("1:/f_1.dat"), FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if (Fatfs_code)
    {
        if (Fatfs_code == FR_EXIST)
        {
            usb_echo("File exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            usb_host_msd_fatfs_test_done();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("test f_write......");
    for (index = 0; index < 58; ++index)
    {
        test_buffer[index] = 'A' + index;
    }
    test_buffer[58] = '\r';
    test_buffer[59] = '\n';
    Fatfs_code      = f_write(&File, test_buffer, 60, (UINT *)&result_size);
    if ((Fatfs_code) || (result_size != 60))
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    Fatfs_code = f_sync(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_printf......");
    if (f_printf(&File, _T("%s\r\n"), "f_printf test") == EOF)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    Fatfs_code = f_sync(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_puts......");
    if (f_puts(_T("f_put test\r\n"), &File) == EOF)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    Fatfs_code = f_sync(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_putc......");
    test_string = "f_putc test\r\n";
    while (*test_string)
    {
        if (f_putc(*test_string, &File) == EOF)
        {
            usb_echo("error\r\n");
            f_close(&File);
            usb_host_msd_fatfs_test_done();
            return;
        }
        test_string++;
    }
    Fatfs_code = f_sync(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_seek......");
    Fatfs_code = f_lseek(&File, 0);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_gets......");
    test_string = f_gets((TCHAR *)&test_buffer[0], 10, &File);
    usb_echo("%s\r\n", test_string);
    usb_echo("test f_read......");
    Fatfs_code = f_read(&File, test_buffer, 10, (UINT *)&result_size);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    test_buffer[result_size] = 0;
    usb_echo("%s\r\n", test_buffer);
#if _USE_FORWARD && _FS_TINY
    usb_echo("test f_forward......");
    Fatfs_code = f_forward(&File, USB_HostMsdFatfsForward, 10, &result_size);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("\r\n");
#endif
    usb_echo("test f_truncate......");
    Fatfs_code = f_truncate(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        f_close(&File);
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_close......");
    Fatfs_code = f_close(&File);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get File \"f_1.dat\" information:\r\n");
    Fatfs_code = f_stat(_T("1:/f_1.dat"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_host_msd_fatfs_display_file_info(&File_info);
    usb_echo("change \"f_1.dat\" timestamp to 2015.10.1, 12:30:0......");
    File_info.fdate = ((uint32_t)(2015 - 1980) << 9 | 10 << 5 | 1); /* 2015.10.1 */
    File_info.ftime = (12 << 11 | 30 << 5);                         /* 12:30:00 */
    Fatfs_code      = f_utime(_T("1:/f_1.dat"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("change \"f_1.dat\" to readonly......");
    Fatfs_code = f_chmod(_T("1:/f_1.dat"), AM_RDO, AM_RDO);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get File \"f_1.dat\" information:\r\n");
    Fatfs_code = f_stat(_T("1:/f_1.dat"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_host_msd_fatfs_display_file_info(&File_info);
    usb_echo("remove \"f_1.dat\" readonly attribute......");
    Fatfs_code = f_chmod(_T("1:/f_1.dat"), 0, AM_RDO);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get File \"f_1.dat\" information:\r\n");
    Fatfs_code = f_stat(_T("1:/f_1.dat"), &File_info);
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_host_msd_fatfs_display_file_info(&File_info);
    usb_echo("rename \"f_1.dat\" to \"f_2.dat\"......");
    Fatfs_code = f_rename(_T("1:/f_1.dat"), _T("1:/f_2.dat"));
    if (Fatfs_code)
    {
        if (Fatfs_code == FR_EXIST)
        {
            usb_echo("File exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            usb_host_msd_fatfs_test_done();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("delete \"f_2.dat\"......");
    Fatfs_code = f_unlink(_T("1:/f_2.dat"));
    if (Fatfs_code)
    {
        usb_echo("error\r\n");
        usb_host_msd_fatfs_test_done();
        return;
    }
    usb_echo("success\r\n");

    usb_host_msd_fatfs_test_done();
}

static void usb_host_msd_fatfs_test_done(void)
{
    usb_echo("............................test done......................\r\n");
}
