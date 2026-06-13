ccflags-y := 	-Wall					\
		-Wextra					\
		-Wno-missing-field-initializers		\
		-Wno-unused-parameter			\
		-Wformat				\
		-O2					\
		-std=gnu18				\
		-g					\
		-Werror=format-security			\
		-Werror=implicit-function-declaration	

# Указываем, что наш модуль ramdisk.ko собирается из файла main.c
ramdisk-y := main.o

obj-m := ramdisk.o