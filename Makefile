obj-m := pt.o
pt-objs := pt_pagewalk.o pt_mem.o pt_pid.o pt_mem.o pt_dpf.o pt_comm.o

# obj-m:=pt_pagewalk.o

# EXTRA_CFLAGS=-g -Wa,-alh=$<.s -save-temps



.PHONY: all clean

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
