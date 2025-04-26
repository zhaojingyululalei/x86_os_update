#适用于Linux
#script=no 表示禁用QEMU自动配置 tap0 接口的脚本,自己去主机上创建tap接口并配置
sudo qemu-system-i386 -daemonize -m 128M -s -S  \
    -drive file=disk1.img,index=0,media=disk,format=raw \
    -drive file=disk2.img,index=1,media=disk,format=raw \
    -drive file=disk3.img,index=2,media=disk,format=raw \
    -drive file=disk4.img,index=3,media=disk,format=raw \
    -rtc base=localtime \
    -cpu Haswell -smp cores=1,threads=2 \
    -netdev tap,id=mynetdev0,ifname=tap0,script=no -device rtl8139,netdev=mynetdev0,mac=52:54:00:c9:18:27 \

# qemu-system-x86_64 \
# 	-m 4G \
#     -s -S \
# 	-boot c \
# 	-cpu Haswell -smp cores=1,threads=2 \
#     -drive file=disk1.img,index=0,media=disk,format=raw -drive file=disk2.img,index=1,media=disk,format=raw \
    


