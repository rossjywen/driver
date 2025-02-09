obj-m := hello.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules


.PHONY clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f *.mod*
	rm -f modules.order


