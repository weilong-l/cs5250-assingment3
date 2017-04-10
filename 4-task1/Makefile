obj-m = fourMb.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
add:
	sudo mknod /dev/fourmb c 62 0
install:
	sudo rmmod fourMb
	sudo insmod fourMb.ko