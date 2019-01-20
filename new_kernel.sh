cd .output/arch/arm/boot
cat zImage dts/qcom-msm8627-sony-xperia-nicki.dtb > ~/projects/linux-nicki-mainline/.zImage-dtb
cp ~/projects/linux-nicki-mainline/.zImage-dtb "$TEMP"/zImage-dtb
cp "/tmp/postmarketOS-export/boot.img-$DEVICE" "$TEMP/boot.img"
pmbootstrap chroot -- abootimg -u /tmp/mainline/boot.img  -k /tmp/mainline/zImage-dtb
pmbootstrap flasher list_devices
pmbootstrap chroot -- fastboot flash boot /tmp/mainline/boot.img
