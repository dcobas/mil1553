CPU = L865
KERNEL ?= 32

ifeq ($(CPU),L865)
	KVER=2.6.24.7-rt27
	ifeq ($(KERNEL), 32)
		KVER=3.2.43-rt63
	endif
else ifeq ($(CPU),L866)
	KVER=3.6.11.2-rt33.39.el6rt.x86_64
endif

KERNELSRC=/acc/sys/$(CPU)/usr/src/kernels/$(KVER)
LINUX ?= $(KERNELSRC)

GIT_VERSION = $(shell cd $(src) ; git describe --dirty --long)

ccflags-y += -g -Wall
ccflags-y += -DCOMPILE_TIME=$(shell date +%s)
ccflags-y += -DGIT_VERSION=\"$(GIT_VERSION)\"
ccflags-y += -DGIT_BUILD_DIR=\"$(src)\"

obj-m += mil1553.o

all: modules

modules:
	make -C $(LINUX) M=`pwd` KVER=$(KVER) CPU=$(CPU) modules
	rm -rf ./$(CPU)/$(KVER)
	mkdir -p ./$(CPU)/$(KVER)
	mv mil1553.ko ./$(CPU)/$(KVER)/mil1553.ko
	chmod 664 ./$(CPU)/$(KVER)/mil1553.ko
	rm -f *.o

clean:
	make -C $(KERNELSRC) M=`pwd` KVER=$(KVER) CPU=$(CPU) clean
	rm -f decode tdecode

install: ./$(CPU)/$(KVER)/mil1553.ko
	@echo "please specify one of {install-lab|install-oper|install-lhc|install-all}"

install-all: install-lab install-oper install-oplhc

install-lab: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/lab/$(CPU)/$(KVER)/mil1553;

install-oper: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/oper/$(CPU)/$(KVER)/mil1553;

install-oplhc: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/oplhc/$(CPU)/$(KVER)/mil1553;
