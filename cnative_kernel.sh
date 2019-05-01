make ARCH=arm CROSS_COMPILE=arm-none-eabi- clean -j16
make ARCH=arm CROSS_COMPILE=arm-none-eabi- -j16 
cd arch/arm/boot
cat zImage dts/qcom-msm8627-sony-xperia-nicki.dtb > ~/projects/linux-postmarketos-qcom/.zImage-dtb
abootimg -u ~/Desktop/boot.img-sony-nicki -k ../../../.zImage-dtb -c "bootsize=12582912"
sudo fastboot flash boot ~/Desktop/boot.img-sony-nicki

