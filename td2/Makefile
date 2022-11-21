obj-m := modhello.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
install:
	depmod -a
	modprobe modhello
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
