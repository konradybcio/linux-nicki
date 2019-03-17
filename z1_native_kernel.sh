make ARCH=arm CROSS_COMPILE=arm-none-eabi- -j16 
cd arch/arm/boot
cat zImage dts/qcom-msm8974-sony-xperia-honami.dtb > ~/projects/linux-nicki-mainline/.zImage-dtb
abootimg -u ~/Desktop/boot.img-sony-honami -k ../../../.zImage-dtb -c "bootsize=12582912"
sudo fastboot flash boot ~/Desktop/boot.img-sony-honami

