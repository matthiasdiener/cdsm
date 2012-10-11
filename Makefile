obj-m := pt_comm.o
pt_comm-objs := pt_pf_thread.o pt_pagewalk.o pt_mem.o pt_pid.o pt_mem.o


.PHONY: all clean

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
