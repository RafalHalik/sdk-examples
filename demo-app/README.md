## Project Breif 

This is a demo application using LVGL with the vg_lite gpu hardware graphics acceleration. Also includes
USB host support allowing for USB updates through the help of MCUboot. MCUboot being the bootloader required
for USB update functionally to be avalible.

The application has a simple screen with a few moving gauges using the animation callback feature of LVGL.

Custom USB stack implemeted in the project to support USB host as zephyr does not support USB host as of 
writing this applicaton and custom VG_lite implementation due to the same reason. 20/11/2024

# Project not building

If your project is not building some commands to run before rebuilding:

> truncate -s 0 /workdir/modules/hal/nxp/mcux/mcux-sdk/components/osa/fsl_os_abstraction_zephyr.c
> truncate -s 0 /workdir/modules/hal/nxp/mcux/middleware/mcux-sdk-middleware-usb/phy/usb_phy.c
> sed -i '8a #define CONFIG_INPUT_GT911_MAX_TOUCH_POINTS 1' /workdir/zephyr/drivers/input/input_gt911.c
> cp /workdir/projects/rafal.vg_lite_dash/vglite/inc/vg_lite.h /workdir/modules/lib/gui/lvgl/src/draw/nxp/vglite/

# west build commands

west build -b mimxrt1160_evk/mimxrt1166/cm7 -- -DSHIELD=rk055hdmipi4ma0

west build -b mimxrt1160_evk/mimxrt1166/cm7 --pristine -- -DSHIELD=rk055hdmipi4ma0

# MCU boot

west sign -t imgtool -d build --no-hex -- --version 1.0.0 --pad --key /workdir/bootloader/mcuboot/root-rsa-2048.pem

west flash -f build/zephyr/zephyr.signed.bin

# J-Link Help

flashing file from JLinkExe

J-Link> exec EnableEraseAllFlashBanks

J-Link> LoadFile /workdir/rafal.vg_lite_dash/build/zephyr/zephyr.bin 0x30000000 

reset

## Project caveats

Project requires some modification to the zephyr sources before it will build. 
The main changes is adding the file from vglite dir into the zephyr sources of vg lite
due to the vg_lite.h file not existing in the current zephyr sources dirs and will not 
find the local in project file so a duplicate needs to be placed here: 
Destination: /workdir/modules/lib/gui/lvgl/src/draw/nxp/vglite

This essential file can be udpicated using this command in vscode intergrated terminal:
> sudo cp /workdir/vg_lite_dash/vglite/inc/vg_lite.h /workdir/modules/lib/gui/lvgl/src/draw/nxp/vglite 

The other major change required to get the project running is to remove a lot of the init for lvgl
module due to this being automatically run before the app loads it causes issues and i have not found 
a way to by pass this and still include lvgl in the project from the modules list. 

This step is KEY to get the correct thing displaying as otherwise the screen will be blank its easy to 
forget this step as the project will still build and flash with this step omitted. 

This is how your lvgl_init(void) function needs to look like in dir:
/workdir/zephyr/modules/lvgl/lvgl.c

* static int lvgl_init(void)
* {
* 	const struct device *display_dev = DEVICE_DT_GET(DISPLAY_NODE);
* 
* 	int err = 0;
* 
* 	if (!device_is_ready(display_dev)) {
* 		LOG_ERR("Display device not ready.");
* 		return -ENODEV;
* 	}
* 
* #ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
* 	lvgl_heap_init();
* #endif
* 
* 	lv_init();
*   return 0;
* }

This is only a temoprary solution till something more maintainable is found...


## If project is not flashing with "west flash"

If the application is building fine however the code is not being loaded onto the dev
board this might mean the JLink is now in a broken state (not sure what causes this)

There is currently only one reliable workaround for this issue, using JFlash V....
you are able to clear the flash memory and load a new application this however is 
not ideal due to the fact you must take the generated binary and give it to JFlash
you can find it in the mounted volume but I have just been downloading into a known
location tagging its version and drag and dropping into the JFlash utility to get binary
ready.

# Using JFlash 
                                                                               
Once the binary is in place and you can see that bin map starting from address 0x30000000
clear flash. Using shortcuts this is F4 however this function is also accessible from the

tool bar at the top:
>>> Target->Manual Programming->Erase Chip          *ALTERNATIVLY* F4

Next flash device:
>>> Target->Manual Programming->Program             *ALTERNATIVLY* F5

Now run app:
>>> Target->Manual Programming->Start Application   *ALTERNATIVLY* F9





## PRobs useless but will leave here just in case 


uart:~$ alling timer callback: 0x3001b995
timer callback 0x3001b995 finished
Time taken: 1:538
m6c6calling timer callback: 0x3001b995
timer callback 0x3001b995 finished
Time taken: 1:477
calling timer callback: 0x3001b995
timer callback 0x3001b995 finished
Time taken: 1:543
calling timer callback: 0x3001b995
timer callback 0x3001b995 finished
Time taken: 1:539
calling timer callback: 0x3001b995



sudo rm mcux/mcux-sdk/boards/evkmimxrt1060/xip/evkmimxrt1060_flexspi_nor_config.c
sudo rm mcux/mcux-sdk/components/video/display/dc/fsl_dc_fb.h
sudo rm mcux/mcux-sdk/components/video/display/dc/lcdifv2/driver_dc-fb-lcdifv2.cmake
sudo rm mcux/mcux-sdk/components/video/display/dc/lcdifv2/fsl_dc_fb_lcdifv2.c
sudo rm mcux/mcux-sdk/components/video/display/dc/lcdifv2/fsl_dc_fb_lcdifv2.h
sudo rm mcux/mcux-sdk/components/video/display/hx8394/driver_display-hx8394.cmake
sudo rm mcux/mcux-sdk/components/video/driver_video-common.cmake
sudo rm mcux/mcux-sdk/devices/MIMXRT1062/MIMXRT1062.h
sudo rm mcux/mcux-sdk/devices/MIMXRT1062/drivers/fsl_clock.c
sudo rm mcux/mcux-sdk/drivers/inputmux/fsl_inputmux.h
sudo rm mcux/mcux-sdk/drivers/lcdifv2/driver_lcdifv2.cmake
sudo rm mcux/mcux-sdk/utilities/debug_console/fsl_debug_console.h
sudo rm mcux/middleware/mcux-sdk-middleware-usb/device/usb_device_khci.c
sudo rm mcux/middleware/mcux-sdk-middleware-usb/host/usb_host_khci.c
sudo rm mcux/middleware/mcux-sdk-middleware-usb/include/usb_misc.h
sudo rm mcux/middleware/mcux-sdk-middleware-usb/middleware_usb_host_common_header.cmake
sudo rm mcux/middleware/mcux-sdk-middleware-usb/middleware_usb_host_ehci_MIMXRT1176_cm7.cmake
sudo rm mcux/middleware/mcux-sdk-middleware-usb/middleware_usb_host_msd.cmake
sudo rm mcux/middleware/mcux-sdk-middleware-usb/middleware_usb_host_stack_MIMXRT1166_cm7.cmake
sudo rm mcux/middleware/mcux-sdk-middleware-usb/middleware_usb_phy.cmake
sudo rm mcux/middleware/mcux-sdk-middleware-usb/phy/usb_phy.c


sudo rm src/core/lv_refr.c
sudo rm src/draw/nxp/pxp/lv_gpu_nxp_pxp_osa.c