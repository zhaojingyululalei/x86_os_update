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
,5M,5
,2M,83
,1M,83
EOF

# 使用查找到的空闲 loop 设备并扫描分区
sudo losetup $LOOP --partscan $DISK2_NAME
ls ${LOOP}p*

# 格式化各个分区为 Minix 文件系统
sudo mkfs.minix -1 -n 14 ${LOOP}p1   # 主分区1
sudo mkfs.minix -1 -n 14 ${LOOP}p2   # 主分区2
sudo mkfs.minix -1 -n 14 ${LOOP}p3   # 主分区3
sudo mkfs.minix -1 -n 14 ${LOOP}p5   # 逻辑分区1
sudo mkfs.minix -1 -n 14 ${LOOP}p6   # 逻辑分区2

# sudo rm *.txt -f
#     echo "part root direcotry file..." > /mnt/hello.txt
#    sudo chown ${USER} /mnt/hello.txt
# 挂载各个分区，创建目录并写入测试文件
for i in 1 2 3 5 6; do
    sudo mount ${LOOP}p$i /mnt
    sudo rm -rf /mnt/test_dir
    sudo chown ${USER} /mnt
    mkdir /mnt/test_dir
    echo "Test file for partition $i" > /mnt/test_dir/test.txt
    mkdir /mnt/test_dir/test1
    echo "Test file for partition $i" > /mnt/test_dir/test1/test1.txt
    mkdir /mnt/test_dir/test2
    echo "Test file for partition $i" > /mnt/test_dir/test2/test2.txt
    mkdir /mnt/test_dir/test3
    echo "Test file for partition $i" > /mnt/test_dir/test3/test3.txt
    mkdir /mnt/test_dir/test1/subtest1
    echo "Test file for partition $i" > /mnt/test_dir/test1/subtest1/test1.txt
    mkdir /mnt/test_dir/test2/subtest2
    echo "Test file for partition $i" > /mnt/test_dir/test2/subtest2/test2.txt
    mkdir /mnt/test_dir/test3/subtest3
    echo "Test file for partition $i" > /mnt/test_dir/test3/subtest3/test3.txt
    mkdir /mnt/test_dir/mnt
    sudo umount /mnt
done
#sudo chown ${USER} /mnt/test_dir/test.txt

# for i in 1 2 3 5 6; do
#     sudo mount ${LOOP}p$i /mnt
#     sudo rm -rf /mnt/test_dir
#     sudo chown ${USER} /mnt
    
#     # 创建测试目录
#     mkdir /mnt/test_dir
    
#     # 创建 5KB 的测试文件（填充随机数据）
#     dd if=/dev/urandom of=/mnt/test_dir/test.bin bs=1024 count=5
    
#     # 检查文件大小（可选）
#     #ls -lh /mnt/test_dir/test.bin
    
#     # 确保权限正确
#     sudo chown ${USER} /mnt/test_dir/test.bin
    
#     # 卸载分区
#     sudo umount /mnt
# done

# 将 disk2.img 复制到 disk3.img 和 disk4.img
cp $DISK2_NAME $DISK3_NAME
cp $DISK2_NAME $DISK4_NAME
# 清理挂载的设备
sudo losetup -d $LOOP



# 设备       启动  起点  末尾  扇区 大小 Id 类型
# disk2.img1       2048  4095  2048   1M 83 Linux
# disk2.img2       4096  6143  2048   1M 83 Linux
# disk2.img3       6144  8191  2048   1M 83 Linux
# disk2.img4       8192 18431 10240   5M  5 扩展
# disk2.img5      10240 14335  4096   2M 83 Linux
# disk2.img6      16384 18431  2048   1M 83 Linux


# 写boot区，定位到磁盘开头，写1个块：512字节
dd if=boot.bin of=$DISK1_NAME bs=512 conv=notrunc count=1

# 写loader区，定位到磁盘第2个块， seek指定在of中的偏移量， bs是单位
dd if=loader.bin of=$DISK1_NAME bs=512 conv=notrunc seek=1

# 写kernel区，定位到磁盘第100个块
dd if=kernel.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=100

# #写shell
dd if=shell.x.elf of=$DISK1_NAME bs=512 conv=notrunc seek=10240