# x86_64_os
# 调试方法
    1.启动qemu -s -S会使虚拟机等待gdb连接  qemu默认启动gdb服务在127.0.0.1:1234
    2.点击运行调试，会依次执行
        1）执行make命令，编译代码
        2）将elf，bin等文件写入磁盘映像
        3）gdb连接qemu的gdb服务 (target remote 127.0.0.1:1234)
            gdb进行一些设置gdb-set disassembly-flavor intel等
            (gdb) add-symbol-file 加载要调试的elf文件，里面包含了调试信息
            (gdb) break *0x7c00 设置断点到0x7c00处
    3.硬盘查看工具，安装Hex Editor插件
    4.build目录下有反汇编，以及elf的解析文件
    

    

符号表里的信息为啥都是相对地址?_start 地址是0，链接时我都加了 -Ttext = 0x8000