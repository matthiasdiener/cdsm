obj-m := spcd.o
spcd-objs := pagefault_thread.o mem.o pid.o mem.o sharing.o probes.o spcd_main.o


.PHONY: all clean

all:
	@mkdir -p obj
	@mv -f obj/* .
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@mv -f *.o modules.order Module.symvers spcd.mod.c obj

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
