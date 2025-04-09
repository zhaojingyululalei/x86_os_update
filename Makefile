
all:
	make -C source/boot -j12
	make -C source/loader -j12
	make -C source/lib/string -j12
	make -C source/lib/algrithm -j12
	make -C source/lib/std -j12 
	make -C source/kernel -j12
	make -C source/app/shell -j12

clean:
	-rm ./image/*.img 
	make clean -C source/boot
	make clean -C source/loader
	make clean -C source/lib/string
	make clean -C source/lib/algrithm
	make clean -C source/lib/std
	make clean -C source/kernel
	make clean -C source/app/shell