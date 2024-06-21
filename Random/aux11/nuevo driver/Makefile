ccflags-y := -Wall -std=gnu99

obj-m := DRIVERNAME.o
DRIVERNAME-objs := kmutex.o DRIVERNAME-impl.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

#include $(KDIR)/.config

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

DRIVERNAME-impl.o kmutex.o: kmutex.h

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
