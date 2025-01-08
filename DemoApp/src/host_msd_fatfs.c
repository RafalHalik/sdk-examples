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
    USB_update_mount_drive = 0,
    USB_update_read_file,
    USB_update_write_to_flash,
    USB_update_move_to_next_section,
    USB_check_image,
    USB_update_idle
} usb_update_state;

static uint8_t testBuffer[(FF_MAX_SS > 256) ? FF_MAX_SS : 256]; 

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


static void USB_HostMsdFatfsTestDone(void);

static void USB_HostMsdFatfsTest(usb_host_msd_fatfs_instance_t *msdFatfsInstance);

 /*!
  * @brief host msd control transfer callback.
  *
  * This function is used as callback function for control transfer .
  *
  * @param param      the host msd fatfs instance pointer.
  * @param data       data buffer pointer.
  * @param dataLength data length.
  * @status           transfer result status.
  */
void USB_HostMsdControlCallback(void* param, uint8_t* data, uint32_t dataLength, usb_status_t status);

/*!
 * @brief display file information.
 */
static void USB_HostMsdFatfsDisplayFileInfo(FILINFO* fileInfo);

/*!
 * @brief list files and sub-directory in one directory, the function don't check all sub-directories recursively.
 */
static FRESULT USB_HostMsdFatfsListDirectory(const TCHAR* path);



/*******************************************************************************
 * Veethree Fnc
 ******************************************************************************/

 /*!
  * @brief Unblocks/block flash write thread
  *
  * This function sets a flag to either block or unblock the write flash task
  * This is called when a USB drive is attached enumerated and mounted with
  * update file present.
  *
  * @param updating  Flag, if USB drive is set up and ready for reading
  */
extern void start_update_process(bool updating);

/*!
 * @brief changes value on update screens progress bar to indicate amount left
 *
 * Takes the size of the file read and total bytes of data read, calculating a %
 * which is then displayed on screen in the form a LVGL bar object
 *
 * @param fileSizeInBytes size of the file being read
 * @param bytesRead amount of data read from usb so far in bytes
 */
extern void update_progress_bar(lv_ui *ui, uint32_t fileSizeInBytes, size_t bytesRead);


/*!
 * @brief Takes read result from fatfs and writes it to flash
 *
 * After fatfs reads the max amount of bytes it can hold in its buffer this function is called
 * to write the buffered data to flash in chuncks that can be user altered with the #define THROUGHPUT_BUFFER_SIZE
 *
 * @param bytesLeftToRead total data to be written to memory
 */
void write_to_flash(uint32_t bytesLeftToRead);


extern void display_read_error(lv_ui *ui, void *displayString);

/*!
 * @brief very basic image validity check
 */
void check_image(lv_ui *ui);


/*******************************************************************************
 * Variables
 ******************************************************************************/

static struct flash_img_context flash_ctx;
static usb_update_state usb_update = USB_update_idle;

static bool ReadyToRead = false;
static bool FileWrittenToFlash = false;

static FRESULT fatfsCode;
static FIL file;
static FILINFO fileInfo;
static uint8_t driverNumberBuffer[3];

 /*! @brief msd class handle array for fatfs */
extern usb_host_class_handle g_UsbFatfsClassHandle;

usb_host_msd_fatfs_instance_t g_MsdFatfsInstance; /* global msd fatfs instance */
static FATFS fatfs;
/* control transfer on-going state. It should set to 1 when start control transfer, it is set to 0 in the callback */
volatile uint8_t controlIng;
/* control transfer callback status */
volatile usb_status_t controlStatus;

#define THROUGHPUT_BUFFER_SIZE (64 * 1024) //(512) //(16 * 1024) //(64 * 1024)
static uint32_t testThroughputBuffer[THROUGHPUT_BUFFER_SIZE / 4];


/*******************************************************************************
 * Code
 ******************************************************************************/



void USB_HostMsdControlCallback(void* param, uint8_t* data, uint32_t dataLength, usb_status_t status)
{
    usb_host_msd_fatfs_instance_t* msdFatfsInstance = (usb_host_msd_fatfs_instance_t*)param;

    if (msdFatfsInstance->runWaitState == kUSB_HostMsdRunWaitSetInterface) /* set interface finish */
    {
        msdFatfsInstance->runWaitState = kUSB_HostMsdRunIdle;
        msdFatfsInstance->runState = kUSB_HostMsdRunMassStorageTest;
    }
    controlIng = 0;
    controlStatus = status;
}


static void USB_HostMsdFatfsDisplayFileInfo(FILINFO* fileInfo)
{
    char* fileName;
    fileName = fileInfo->fname;
    /* note: if this file/directory don't have one attribute, '_' replace the attribute letter ('R' - readonly, 'H' -
     * hide, 'S' - system) */
    usb_echo("    %s - %c%c%c - %s - %dBytes - %d-%d-%d %d:%d:%d\r\n", (fileInfo->fattrib & AM_DIR) ? "dir" : "fil",
        (fileInfo->fattrib & AM_RDO) ? 'R' : '_', (fileInfo->fattrib & AM_HID) ? 'H' : '_',
        (fileInfo->fattrib & AM_SYS) ? 'S' : '_', fileName, (fileInfo->fsize),
        (uint32_t)((fileInfo->fdate >> 9) + 1980) /* year */,
        (uint32_t)((fileInfo->fdate >> 5) & 0x000Fu) /* month */, (uint32_t)(fileInfo->fdate & 0x001Fu) /* day */,
        (uint32_t)((fileInfo->ftime >> 11) & 0x0000001Fu) /* hour */,
        (uint32_t)((fileInfo->ftime >> 5) & 0x0000003Fu) /* minute */,
        (uint32_t)(fileInfo->ftime & 0x0000001Fu) /* second */
    );
}

static FRESULT USB_HostMsdFatfsListDirectory(const TCHAR* path)
{
    FRESULT fatfsCode = FR_OK;
    FILINFO fileInfo;
    DIR dir;
    uint8_t outputLabel = 0;

    fatfsCode = f_opendir(&dir, path);
    if (fatfsCode)
    {
        return fatfsCode;
    }
    while (1)
    {
        fatfsCode = f_readdir(&dir, &fileInfo);
        if ((fatfsCode) || (!fileInfo.fname[0]))
        {
            break;
        }
        outputLabel = 1;
        USB_HostMsdFatfsDisplayFileInfo(&fileInfo);
    }
    if (!outputLabel)
    {
        // usb_echo("\r\n");
    }

    return fatfsCode;
}

void USB_HostMsdTask(void* arg)
{
    usb_status_t status;
    usb_host_msd_fatfs_instance_t* msdFatfsInstance = (usb_host_msd_fatfs_instance_t*)arg;
    if (msdFatfsInstance->deviceState != msdFatfsInstance->prevDeviceState)
    {
        msdFatfsInstance->prevDeviceState = msdFatfsInstance->deviceState;
        switch (msdFatfsInstance->deviceState)
        {
        case kStatus_DEV_Idle:    
         usb_echo("kStatus_DEV_Idle \r\n");      
            break;

        case kStatus_DEV_Attached: /* deivce is attached and numeration is done */
         usb_echo("kStatus_DEV_Attached \r\n");
            status = USB_HostMsdInit(msdFatfsInstance->deviceHandle,
                &msdFatfsInstance->classHandle); /* msd class initialization */
            g_UsbFatfsClassHandle = msdFatfsInstance->classHandle;
            if (status != kStatus_USB_Success)
            {
                usb_echo("usb host msd init fail\r\n");
                return;
            }
            msdFatfsInstance->runState = kUSB_HostMsdRunSetInterface;
            break;

        case kStatus_DEV_Detached: /* device is detached */
          usb_echo("kStatus_DEV_Detached \r\n");
            msdFatfsInstance->deviceState = kStatus_DEV_Idle;
            msdFatfsInstance->runState = kUSB_HostMsdRunIdle;
            USB_HostMsdDeinit(msdFatfsInstance->deviceHandle,
                msdFatfsInstance->classHandle); /* msd class de-initialization */
            msdFatfsInstance->classHandle = NULL;          
            start_update_process(false);
            usb_echo("mass storage device detached\r\n");
            break;

        default:
            break;
        }
    }

    /* run state */
    switch (msdFatfsInstance->runState)
    {
    case kUSB_HostMsdRunIdle:
        break;

    case kUSB_HostMsdRunSetInterface: /* set msd interface */
    usb_echo("kUSB_HostMsdRunSetInterface \r\n");
        msdFatfsInstance->runState = kUSB_HostMsdRunIdle;
        msdFatfsInstance->runWaitState = kUSB_HostMsdRunWaitSetInterface;
        status = USB_HostMsdSetInterface(msdFatfsInstance->classHandle, msdFatfsInstance->interfaceHandle, 0,
            USB_HostMsdControlCallback, msdFatfsInstance);
        if (status != kStatus_USB_Success)
        {
            // usb_echo("set interface fail\r\n");
        }
        break;

    case kUSB_HostMsdRunMassStorageTest: /* set interface succeed */
    usb_echo("kUSB_HostMsdRunMassStorageTest \r\n");
        usb_update = USB_update_mount_drive;
        ReadyToRead = true;
        start_update_process(true);
        msdFatfsInstance->runState = kUSB_HostMsdRunIdle;      
        break;
    case kUSB_HostMsdRunUpdate:
    usb_echo("kUSB_HostMsdRunUpdate \r\n");
        start_update_process(true);
        usb_echo("USB update case... \r\n");
        msdFatfsInstance->runState = kUSB_HostMsdRunIdle;
        break;
    default:
        break;
    }
}



void usb_update_task(lv_ui *ui)
{
    static uint32_t bytesLeftToRead = 0;
    static uint32_t resultSize = 1;
    static uint8_t fileReadCounter = 0;
    static size_t resultSizeTotal = 0;

    static uint8_t DEBUG_number_of_writes = 0;

    switch (usb_update)
    {
    case USB_update_mount_drive:       
        // Reinitalize varibles incase somethign went wrong and update had to be attempted again...
        bytesLeftToRead = 0;
        resultSize = 1;
        fileReadCounter = 0;
        resultSizeTotal = 0;
        DEBUG_number_of_writes = 0;

        sprintf((char*)&driverNumberBuffer[0], "%c:", USBDISK + '0');
        fatfsCode = f_mount(&fatfs, (char const*)&driverNumberBuffer[0], 0);
        if (fatfsCode)
        {
             usb_echo("error mounting %d\r\n", fatfsCode);
            return;
        }

        fatfsCode = USB_HostMsdFatfsListDirectory((char const*)&driverNumberBuffer[0]);
        if (fatfsCode)
        {
             usb_echo("error reading drive %d\r\n", fatfsCode);
            return;
        }

        fatfsCode = f_open(&file, _T("1:/zephyr~1.bin"), FA_READ | FA_OPEN_EXISTING);
        if (fatfsCode)
        {
            usb_echo("error file no able to be opended\r\n");
        }
        else
        {
            fatfsCode = f_stat(_T("1:/zephyr~1.bin"), &fileInfo);
            if (fatfsCode)
            {
                return;
            }
        }
        flash_img_init_id(&flash_ctx, SLOT1_ID);
        flash_area_erase(flash_ctx.flash_area, SLOT1_ID, flash_ctx.flash_area->fa_size);

        bytesLeftToRead = fileInfo.fsize;
        usb_update = USB_update_read_file;
        break;
    case USB_update_read_file:
            fatfsCode = f_read(&file, testThroughputBuffer, THROUGHPUT_BUFFER_SIZE, &resultSize);
            if (fatfsCode)
            {
                usb_echo("read error %d \r\n", fatfsCode);                        
                update_failed_clean_up("Error Reading! try again...");
                return;
            }           

            bytesLeftToRead -= resultSize;
            usb_update = USB_update_write_to_flash;
        break;
    case USB_update_write_to_flash:       
        write_to_flash(bytesLeftToRead);
        DEBUG_number_of_writes++;
        resultSizeTotal += resultSize;
        update_progress_bar(ui, fileInfo.fsize, resultSizeTotal);

        usb_update = USB_update_move_to_next_section;
        break;
    case USB_update_move_to_next_section:     
        fatfsCode = f_lseek(&file, resultSizeTotal);
        if (fatfsCode)
        {
            usb_echo("seek error\r\n");
            usb_update = USB_update_idle;
            f_close(&file);
            return;
        }
        if ((flash_img_bytes_written(&flash_ctx) >= fileInfo.fsize) || (bytesLeftToRead == 0))
        {
            usb_update = USB_check_image;
        }
        else
        {
            usb_update = USB_update_read_file;
        }
        break;
    case USB_check_image:
        usb_echo("USB_check_image \r\n");
        check_image(ui);
        usb_update = USB_update_idle;
        ReadyToRead = false;
        break;
    case USB_update_idle:
        // wait
        if (ReadyToRead == true)
        {
            usb_update = USB_update_mount_drive;
        }
        else
        {
            k_msleep(5);
            usb_update = USB_update_idle;
        }        
        break;
    default:
        usb_echo("Unknown state... \r\n");
        break;
    }

}

void update_failed_clean_up(lv_ui *ui, void* displayString)
{
    display_read_error(ui, displayString);   
    ReadyToRead = false;
    FileWrittenToFlash = false;
    usb_update = USB_update_idle; 
    f_close(&file);
}


void check_image(lv_ui *ui)
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
        update_failed_clean_up(ui, "Image invalid...");
        return;
    }

    if (boot_request_upgrade(BOOT_UPGRADE_PERMANENT))
    {
        usb_echo("Upgrade request FAIL \r\n");
        return;
    }
   
    FileWrittenToFlash = true;
}

void write_to_flash(uint32_t bytesLeftToRead)
{  
    if(usb_update == USB_update_write_to_flash)
    {
        int ret = 0;

        bool flush = false;
        if (bytesLeftToRead == 0)
        {
            // whole file read now force flush/write of buffered data before reboot
            flush = true;        
        }
        usb_echo("bytesLeftToRead %d \r\n",bytesLeftToRead);
        ret = flash_img_buffered_write(&flash_ctx, (uint8_t *)testThroughputBuffer, THROUGHPUT_BUFFER_SIZE, flush);

        if (ret)
        {
            usb_echo("bytesLeftToRead %d \r\n",bytesLeftToRead);
            
            usb_echo("flash write failed \r\n");
            return;
        }
    }
}

usb_status_t USB_HostMsdEvent(usb_device_handle deviceHandle,
    usb_host_configuration_handle configurationHandle,
    uint32_t eventCode)
{
    usb_status_t status = kStatus_USB_Success;
    usb_host_configuration_t* configuration;
    uint8_t interfaceIndex;
    usb_host_interface_t* interface;
    uint32_t infoValue = 0U;
    uint8_t id;

    switch (eventCode)
    {
    case kUSB_HostEventAttach:
        /* judge whether is configurationHandle supported */
        configuration = (usb_host_configuration_t*)configurationHandle;
        for (interfaceIndex = 0; interfaceIndex < configuration->interfaceCount; ++interfaceIndex)
        {
            interface = &configuration->interfaceList[interfaceIndex];
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
                    g_MsdFatfsInstance.deviceHandle = deviceHandle;
                    g_MsdFatfsInstance.interfaceHandle = interface;
                    g_MsdFatfsInstance.configHandle = configurationHandle;
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
        if (g_MsdFatfsInstance.configHandle == configurationHandle)
        {
            if ((g_MsdFatfsInstance.deviceHandle != NULL) && (g_MsdFatfsInstance.interfaceHandle != NULL))
            {
                /* the device enumeration is done */
                if (g_MsdFatfsInstance.deviceState == kStatus_DEV_Idle)
                {
                    g_MsdFatfsInstance.deviceState = kStatus_DEV_Attached;

                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDevicePID, &infoValue);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceVID, &infoValue);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceAddress, &infoValue);
                }
                else
                {
                    status = kStatus_USB_Error;
                }
            }
        }
        break;

    case kUSB_HostEventDetach:
        if (g_MsdFatfsInstance.configHandle == configurationHandle)
        {
            /* the device is detached */
            g_UsbFatfsClassHandle = NULL;
            g_MsdFatfsInstance.configHandle = NULL;
            if (g_MsdFatfsInstance.deviceState != kStatus_DEV_Idle)
            {
                usb_update = USB_update_idle;
                usb_echo("status DETACHED \r\n");
                if (FileWrittenToFlash == true)
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



static void USB_HostMsdFatfsTest(usb_host_msd_fatfs_instance_t *msdFatfsInstance)
{
    FRESULT fatfsCode;
    FATFS *fs;
    FIL file;
    FILINFO fileInfo;
    uint32_t freeClusterNumber;
    uint32_t index;
    uint32_t resultSize;
    char *testString;
    uint8_t driverNumberBuffer[3];

    /* time delay */
    for (freeClusterNumber = 0; freeClusterNumber < 10000; ++freeClusterNumber)
    {
        __NOP();
    }

    usb_echo("............................fatfs test.....................\r\n");

    usb_echo("fatfs mount as logiacal driver %d......", USBDISK);
    sprintf((char *)&driverNumberBuffer[0], "%c:", USBDISK + '0');
    fatfsCode = f_mount(&fatfs, (char const *)&driverNumberBuffer[0], 0);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");

#if (FF_FS_RPATH >= 2)
    fatfsCode = f_chdrive((char const *)&driverNumberBuffer[0]);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
#endif

#if FF_USE_MKFS
    MKFS_PARM formatOptions = {FM_SFD | FM_ANY, 0, 0, 0, 0};
    usb_echo("test f_mkfs......");
    fatfsCode = f_mkfs((char const *)&driverNumberBuffer[0], &formatOptions, testBuffer, FF_MAX_SS);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
#endif /* FF_USE_MKFS */

    usb_echo("test f_getfree:\r\n");
    fatfsCode = f_getfree((char const *)&driverNumberBuffer[0], (DWORD *)&freeClusterNumber, &fs);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
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
    usb_echo("    The free size: %dKB, the total size:%dKB\r\n", (freeClusterNumber * (fs->csize) / 2),
             ((fs->n_fatent - 2) * (fs->csize) / 2));

    usb_echo("directory operation:\r\n");
    usb_echo("list root directory:\r\n");
    fatfsCode = USB_HostMsdFatfsListDirectory((char const *)&driverNumberBuffer[0]);
    if (fatfsCode)
    {
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("create directory \"dir_1\"......");
    fatfsCode = f_mkdir(_T("1:/dir_1"));
    if (fatfsCode)
    {
        if (fatfsCode == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            USB_HostMsdFatfsTestDone();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("create directory \"dir_2\"......");
    fatfsCode = f_mkdir(_T("1:/dir_2"));
    if (fatfsCode)
    {
        if (fatfsCode == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            USB_HostMsdFatfsTestDone();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("create sub directory \"dir_2/sub_1\"......");
    fatfsCode = f_mkdir(_T("1:/dir_1/sub_1"));
    if (fatfsCode)
    {
        if (fatfsCode == FR_EXIST)
        {
            usb_echo("directory exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            USB_HostMsdFatfsTestDone();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("list root directory:\r\n");
    fatfsCode = USB_HostMsdFatfsListDirectory(_T("1:"));
    if (fatfsCode)
    {
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("list directory \"dir_1\":\r\n");
    fatfsCode = USB_HostMsdFatfsListDirectory(_T("1:/dir_1"));
    if (fatfsCode)
    {
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("rename directory \"dir_1/sub_1\" to \"dir_1/sub_2\"......");
    fatfsCode = f_rename(_T("1:/dir_1/sub_1"), _T("1:/dir_1/sub_2"));
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("delete directory \"dir_1/sub_2\"......");
    fatfsCode = f_unlink(_T("1:/dir_1/sub_2"));
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");

#if (FF_FS_RPATH >= 2)
    usb_echo("get current directory......");
    fatfsCode = f_getcwd((TCHAR *)&testBuffer[0], 256);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("%s\r\n", testBuffer);
    usb_echo("change current directory to \"dir_1\"......");
    fatfsCode = f_chdir(_T("dir_1"));
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("list current directory:\r\n");
    fatfsCode = USB_HostMsdFatfsListDirectory(_T("."));
    if (fatfsCode)
    {
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("get current directory......");
    fatfsCode = f_getcwd((TCHAR *)&testBuffer[0], 256);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("%s\r\n", testBuffer);
#endif

    usb_echo("get directory \"dir_1\" information:\r\n");
    fatfsCode = f_stat(_T("1:/dir_1"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    USB_HostMsdFatfsDisplayFileInfo(&fileInfo);
    usb_echo("change \"dir_1\" timestamp to 2015.10.1, 12:30:0......");
    fileInfo.fdate = ((2015 - 1980) << 9 | 10 << 5 | 1); /* 2015.10.1 */
    fileInfo.ftime = (12 << 11 | 30 << 5);               /* 12:30:00 */
    fatfsCode      = f_utime(_T("1:/dir_1"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get directory \"dir_1\" information:\r\n");
    fatfsCode = f_stat(_T("1:/dir_1"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    USB_HostMsdFatfsDisplayFileInfo(&fileInfo);

    usb_echo("file operation:\r\n");
    usb_echo("create file \"f_1.dat\"......");
    fatfsCode = f_open(&file, _T("1:/f_1.dat"), FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if (fatfsCode)
    {
        if (fatfsCode == FR_EXIST)
        {
            usb_echo("file exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            USB_HostMsdFatfsTestDone();
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
        testBuffer[index] = 'A' + index;
    }
    testBuffer[58] = '\r';
    testBuffer[59] = '\n';
    fatfsCode      = f_write(&file, testBuffer, 60, (UINT *)&resultSize);
    if ((fatfsCode) || (resultSize != 60))
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    fatfsCode = f_sync(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_printf......");
    if (f_printf(&file, _T("%s\r\n"), "f_printf test") == EOF)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    fatfsCode = f_sync(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_puts......");
    if (f_puts(_T("f_put test\r\n"), &file) == EOF)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    fatfsCode = f_sync(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_putc......");
    testString = "f_putc test\r\n";
    while (*testString)
    {
        if (f_putc(*testString, &file) == EOF)
        {
            usb_echo("error\r\n");
            f_close(&file);
            USB_HostMsdFatfsTestDone();
            return;
        }
        testString++;
    }
    fatfsCode = f_sync(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_seek......");
    fatfsCode = f_lseek(&file, 0);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_gets......");
    testString = f_gets((TCHAR *)&testBuffer[0], 10, &file);
    usb_echo("%s\r\n", testString);
    usb_echo("test f_read......");
    fatfsCode = f_read(&file, testBuffer, 10, (UINT *)&resultSize);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    testBuffer[resultSize] = 0;
    usb_echo("%s\r\n", testBuffer);
#if _USE_FORWARD && _FS_TINY
    usb_echo("test f_forward......");
    fatfsCode = f_forward(&file, USB_HostMsdFatfsForward, 10, &resultSize);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("\r\n");
#endif
    usb_echo("test f_truncate......");
    fatfsCode = f_truncate(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        f_close(&file);
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("test f_close......");
    fatfsCode = f_close(&file);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get file \"f_1.dat\" information:\r\n");
    fatfsCode = f_stat(_T("1:/f_1.dat"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    USB_HostMsdFatfsDisplayFileInfo(&fileInfo);
    usb_echo("change \"f_1.dat\" timestamp to 2015.10.1, 12:30:0......");
    fileInfo.fdate = ((uint32_t)(2015 - 1980) << 9 | 10 << 5 | 1); /* 2015.10.1 */
    fileInfo.ftime = (12 << 11 | 30 << 5);                         /* 12:30:00 */
    fatfsCode      = f_utime(_T("1:/f_1.dat"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("change \"f_1.dat\" to readonly......");
    fatfsCode = f_chmod(_T("1:/f_1.dat"), AM_RDO, AM_RDO);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get file \"f_1.dat\" information:\r\n");
    fatfsCode = f_stat(_T("1:/f_1.dat"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    USB_HostMsdFatfsDisplayFileInfo(&fileInfo);
    usb_echo("remove \"f_1.dat\" readonly attribute......");
    fatfsCode = f_chmod(_T("1:/f_1.dat"), 0, AM_RDO);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");
    usb_echo("get file \"f_1.dat\" information:\r\n");
    fatfsCode = f_stat(_T("1:/f_1.dat"), &fileInfo);
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    USB_HostMsdFatfsDisplayFileInfo(&fileInfo);
    usb_echo("rename \"f_1.dat\" to \"f_2.dat\"......");
    fatfsCode = f_rename(_T("1:/f_1.dat"), _T("1:/f_2.dat"));
    if (fatfsCode)
    {
        if (fatfsCode == FR_EXIST)
        {
            usb_echo("file exist\r\n");
        }
        else
        {
            usb_echo("error\r\n");
            USB_HostMsdFatfsTestDone();
            return;
        }
    }
    else
    {
        usb_echo("success\r\n");
    }
    usb_echo("delete \"f_2.dat\"......");
    fatfsCode = f_unlink(_T("1:/f_2.dat"));
    if (fatfsCode)
    {
        usb_echo("error\r\n");
        USB_HostMsdFatfsTestDone();
        return;
    }
    usb_echo("success\r\n");

    USB_HostMsdFatfsTestDone();
}

static void USB_HostMsdFatfsTestDone(void)
{
    usb_echo("............................test done......................\r\n");
}
