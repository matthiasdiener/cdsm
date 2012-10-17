obj-m := pt.o
pt-objs := pt_pagewalk.o pt_mem.o pt_pid.o pt_mem.o pt_dpf.o pt_comm.o

EXTRA_CFLAGS=-g -Wall
.PHONY: all clean

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
