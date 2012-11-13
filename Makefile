obj-m := spcd.o
spcd-objs := pagefault_thread.o mem.o pid.o mem.o sharing.o probes.o spcd_main.o


.PHONY: all clean

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
