obj-m := spcd.o
spcd-objs := pagefault.o mem.o pid.o mem.o sharing.o probes.o intercept.o map.o topo.o spcd_main.o

SPCD_VER=$(shell git describe)
DATE=$(shell date)

.PHONY: all clean

all:
	@echo "#define SPCD_VERSION \"$(SPCD_VER); $(DATE)\"" > version.h
	@if stat -t obj/* >/dev/null 2>&1; then mv -f obj/* . ; else mkdir -p obj; fi
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@mv -f *.o modules.order Module.symvers spcd.mod.c obj

clean:
	@rm -rf obj/
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
