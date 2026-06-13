# Определяем путь к заголовочным файлам текущего ядра
KDIR := /lib/modules/$(shell uname -r)/build

all: modules test_app

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test_app:
	gcc -Wall -O2 test_app.c -o test_app

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_app