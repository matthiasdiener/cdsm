obj-m := spcd.o
spcd-objs := libspcd.o pagefault.o mem.o pid.o sharing.o probes.o map.o topo.o spcd_main.o map/map_simple.o procfs.o ../libmapping/mapping-greedy.o ../libmapping/topology.o ../libmapping/lib.o ../libmapping/graph.o


include ~/Work/libmapping/make.config.spcd
include ~/Work/libmapping/make.macros

ccflags-y += -g -Wall -D_SPCD -I/home/mdiener/Work/libmapping/ $(LMCFLAGS)

.PHONY: all clean install

all:
	@sync
	@echo "#define SPCD_VERSION \"$(shell git describe); $(shell date)\"" > version.h
	@if stat -t obj/* >/dev/null 2>&1; then mv -f obj/* obj/.*.cmd . ; else mkdir -p obj; fi
	make -j -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@mv -f *.o modules.order Module.symvers spcd.mod.c .*.cmd obj

clean:
	@rm -rf obj/
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


install: all
	@if test $(shell lsmod | grep spcd >/dev/null; echo $$?) -eq 0; then sudo rmmod spcd; fi
	sudo insmod $(PWD)/spcd.ko $(options)
	@dmesg | grep -i "spcd bug"; echo -n
