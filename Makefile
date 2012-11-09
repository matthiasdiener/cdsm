obj-m := spcd.o
spcd-objs := pagewalk.o mem.o pid.o mem.o check_comm.o spcd_main.o


.PHONY: all clean

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
