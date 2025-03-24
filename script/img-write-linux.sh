
dd if=/dev/zero of=disk1.img bs=1M count=50
dd if=/dev/zero of=disk2.img bs=1M count=50
dd if=/dev/zero of=disk3.img bs=1M count=5
dd if=/dev/zero of=disk4.img bs=1M count=5

if [ -f "disk1.vhd" ]; then
    mv disk1.vhd disk1.img
fi

if [ ! -f "disk1.img" ]; then
    echo "error: no disk1.vhd, download it first!!!"
    exit -1
fi

if [ -f "disk2.vhd" ]; then
    mv disk2.vhd disk2.img
fi

if [ ! -f "disk2.img" ]; then
    echo "error: no disk2.vhd, download it first!!!"
    exit -1
fi

export DISK1_NAME=disk1.img

# 写boot区，定位到磁盘开头，写1个块：512字节
dd if=boot.bin of=$DISK1_NAME bs=512 conv=notrunc count=1

# 写loader区，定位到磁盘第2个块， seek指定在of中的偏移量， bs是单位
dd if=loader.bin of=$DISK1_NAME bs=512 conv=notrunc seek=1

# 写kernel区，定位到磁盘第100个块
dd if=kernel.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=100

# #写shell
# dd if=shell.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=10240