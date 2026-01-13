obj-m += mon.o
mon-objs := monitor.o
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

.PHONY: all