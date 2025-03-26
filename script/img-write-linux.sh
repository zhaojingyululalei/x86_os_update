#!/bin/bash

# 创建磁盘映像文件
dd if=/dev/zero of=disk1.img bs=1M count=50
dd if=/dev/zero of=disk2.img bs=1M count=10  # 增大到10MB以容纳所有分区
dd if=/dev/zero of=disk3.img bs=1M count=5
dd if=/dev/zero of=disk4.img bs=1M count=5

# 查找空闲的 loop 设备
LOOP=$(sudo losetup --find)
echo "Using loop device: $LOOP"

export DISK1_NAME=disk1.img
export DISK2_NAME=disk2.img
export DISK3_NAME=disk3.img
export DISK4_NAME=disk4.img

# 对 disk2.img 进行分区：3个主分区 + 1个扩展分区（包含2个逻辑分区）
# 注意：扩展分区需要足够大的空间来包含逻辑分区
echo "label: dos" | sfdisk $DISK2_NAME << EOF
,1M,83
,1M,83
,1M,83
,4M,5
,1M,83
,1M,83
EOF

# 使用查找到的空闲 loop 设备并扫描分区
sudo losetup $LOOP --partscan $DISK2_NAME
ls ${LOOP}p*

# 格式化各个分区为 Minix 文件系统
sudo mkfs.minix ${LOOP}p1   # 主分区1
sudo mkfs.minix ${LOOP}p2   # 主分区2
sudo mkfs.minix ${LOOP}p3   # 主分区3
sudo mkfs.minix ${LOOP}p5   # 逻辑分区1
sudo mkfs.minix ${LOOP}p6   # 逻辑分区2

# 挂载各个分区，创建目录并写入测试文件
for i in 1 2 3 5 6; do
    sudo mount ${LOOP}p$i /mnt
    sudo rm -rf /mnt/test_dir
    sudo mkdir /mnt/test_dir
    sudo chown ${USER} /mnt/test_dir
    echo "Test file for partition $i" > /mnt/test_dir/test.txt
    sudo chown ${USER} /mnt/test_dir/test.txt
    sudo umount /mnt
done

# 将 disk2.img 复制到 disk3.img 和 disk4.img
cp $DISK2_NAME $DISK3_NAME
cp $DISK2_NAME $DISK4_NAME
# 清理挂载的设备
sudo losetup -d $LOOP


# 设备       启动  起点  末尾  扇区 大小 Id 类型
# disk2.img1       2048  4095  2048   1M 83 Linux
# disk2.img2       4096  6143  2048   1M 83 Linux
# disk2.img3       6144  8191  2048   1M 83 Linux
# disk2.img4       8192 16383  8192   4M  5 扩展
# disk2.img5      10240 12287  2048   1M 83 Linux
# disk2.img6      14336 16383  2048   1M 83 Linux



# 写boot区，定位到磁盘开头，写1个块：512字节
dd if=boot.bin of=$DISK1_NAME bs=512 conv=notrunc count=1

# 写loader区，定位到磁盘第2个块， seek指定在of中的偏移量， bs是单位
dd if=loader.bin of=$DISK1_NAME bs=512 conv=notrunc seek=1

# 写kernel区，定位到磁盘第100个块
dd if=kernel.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=100

# #写shell
# dd if=shell.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=10240