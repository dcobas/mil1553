CPU=L865

ifeq ($(CPU),L865)
# KVER=2.6.24.7-rt21
# KVER=2.6.24.7-rt27
# KVER=2.6.29.4-rt15
# KVER=2.6.31.6-rt19
KVER=2.6.24.7-rt27
endif

ifeq ($(CPU),L866)
# KVER=2.6.32-131.17.1.el6.x86_64
KVER=2.6.33.9-rt31.75.el6rt.x86_64
endif

KERNELSRC=/acc/sys/$(CPU)/usr/src/kernels/$(KVER)

CFLAGS=-g -Wall

modules: 
	make -C $(KERNELSRC) M=`pwd` KVER=$(KVER) CPU=$(CPU) modules
	rm -rf ./$(CPU)/$(KVER)
	mkdir ./$(CPU)/$(KVER)
	mv mil1553.ko ./$(CPU)/$(KVER)/mil1553.ko
	chmod 664 ./$(CPU)/$(KVER)/mil1553.ko
	rm -f *.o

clean:
	rm -f mil1553.ko
	rm -f ./$(CPU)/$(KVER)/mil1553.ko
	rm -f *.o
	rm -f Module.symvers

install: ./$(CPU)/$(KVER)/mil1553.ko
	@echo "please specify one of {install-lab|install-oper|install-lhc|install-all}"

install-all: install-lab install-oper install-oplhc

install-lab: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/lab/$(CPU)/$(KVER)/mil1553;

install-oper: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/oper/$(CPU)/$(KVER)/mil1553;

install-oplhc: ./$(CPU)/$(KVER)/mil1553.ko
	dsc_install ./$(CPU)/$(KVER)/mil1553.ko /acc/dsc/oplhc/$(CPU)/$(KVER)/mil1553;
