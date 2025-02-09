# Ross 2020/2/22
#
#

PWD := `pwd`
KERNEL_DIR := /lib/modules/`uname -r`/build

obj-m += scull.o

all:
	make -C $(KERNEL_DIR) M=$(PWD)


.PHONY clean:
	rm -f *.ko
	rm -f *.mod*
	rm -f *.order
	rm -f *.symvers
	rm -f *.o
	rm -f *.a

