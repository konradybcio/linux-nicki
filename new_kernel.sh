pmbootstrap chroot -- apk add abootimg android-tools mkbootimg dtbtool
cd .output/arch/arm/boot
cat zImage dts/qcom-msm8627-sony-xperia-nicki.dtb > ~/projects/mainline_nicki/.zImage-dtb
cp ~/projects/mainline_nicki/.zImage-dtb "$TEMP"/zImage-dtb
cp "/tmp/postmarketOS-export/boot.img-$DEVICE" "$TEMP/boot.img"
