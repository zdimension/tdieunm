obj-m := cutbuf.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
install:
	insmod cutbuf.ko
uninstall:
	rmmod cutbuf.ko
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
log: 
	tail -f /var/log/messages
