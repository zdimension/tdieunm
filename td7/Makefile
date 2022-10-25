TARGET = td7

obj-m := $(TARGET).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
KDIR = /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

ins: rm all
	@insmod ./$(TARGET).ko
	@echo "Loading module (messages in /var/log/messages)"

probe: all
	cp *.ko /lib/modules/$(shell uname -r)/kernel
	depmod -a
	@modprobe $(TARGET)
	@echo "Loading module (messages in /var/log/messages)"

rm:
	@echo "Unloading module (messages in /var/log/messages)"
	@-rmmod $(TARGET) 2> /dev/null
	@if [ "$?" = "0" ] ; then echo "Module $(TARGET) unloaded." ; else echo "Module $(TARGET) not loaded." ; fi