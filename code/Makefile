obj-m += mon.o
mon-objs := monitor.o encrypt.o
KDIR ?= /lib/modules/$(shell uname -r)/build
ccflags-y += -DDEBUG  # Раскомментируйте для отладки

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: all
	sudo insmod mon.ko

uninstall:
	sudo rmmod mon 2>/dev/null || true

reload: uninstall install

.PHONY: all clean install uninstall reload check info