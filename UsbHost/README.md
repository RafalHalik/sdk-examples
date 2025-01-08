# Breif 

USB update application, this application intializes the usb host from usb_host_veethree enabled with CONFIG_USB_VEETHREE=y.
This allows for the a usb to be plugged into the device and an automatically start downloading the update and writing it to
flash on the device. The application is a minimal one so no display is enabled using lvgl the uart output will show the progress 
of the update and if anything goes wrong.

# west commands
west build -b mimxrt1160_evk/mimxrt1166/cm7 --pristine -- -DSHIELD=rk055hdmipi4ma0
west sign -t imgtool -d build --no-hex -- --version 1.0.0 --pad --key /workdir/bootloader/mcuboot/root-rsa-2048.pem

west flash -f build/zephyr/zephyr.signed.bin

