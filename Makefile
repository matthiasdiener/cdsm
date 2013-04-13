obj-m := spcd.o
spcd-objs := libspcd.o pagefault.o mem.o pid.o sharing.o probes.o map.o topo.o procfs.o spcd_main.o map/map_simple.o map/map_drake.o ../libmapping/mapping-greedy.o ../libmapping/topology.o ../libmapping/lib.o ../libmapping/graph.o

ccflags-y += -g -Wall -D_SPCD -I/home/mdiener/Work/libmapping/

options = ""

SPCD_VER=$(shell git describe)
DATE=$(shell date)

.PHONY: all clean

all:
	@sync
	@echo "#define SPCD_VERSION \"$(SPCD_VER); $(DATE)\"" > version.h
	@if stat -t obj/* >/dev/null 2>&1; then mv -f obj/* obj/.*.cmd . ; else mkdir -p obj; fi
	make -j4 -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@mv -f *.o modules.order Module.symvers spcd.mod.c .*.cmd obj

clean:
	@rm -rf obj/
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


install: all
	-sudo rmmod spcd
	sudo insmod ${PWD}/spcd.ko ${options}
	-dmesg | grep -i bug
