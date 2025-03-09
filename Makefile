
all:
	make -C source/boot
	make -C source/loader
	make -C source/lib/string
	make -C source/lib/algrithm
	make -C source/lib/std
	make -C source/kernel
	# make -C source/app/shell

clean:
	-rm ./image/*.img 
	make clean -C source/boot
	make clean -C source/loader
	make clean -C source/lib/string
	make clean -C source/lib/algrithm
	make clean -C source/lib/std
	make clean -C source/kernel
	# make clean -C source/app/shell