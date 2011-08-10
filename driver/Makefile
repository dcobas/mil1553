CPU=L865

# KVER=2.6.24.7-rt21
# KVER=2.6.24.7-rt27
# KVER=2.6.29.4-rt15
# KVER=2.6.31.6-rt19
ifeq ($(CPU),L865)
KVER=2.6.24.7-rt27
endif
ifeq ($(CPU),L866)
KVER=2.6.33.9-rt31.66.el6rt.x86_64
endif

KERNELSRC=/acc/sys/$(CPU)/usr/src/kernels/$(KVER)

CFLAGS=-g -Wall

modules: 
	make -C $(KERNELSRC) M=`pwd` KVER=$(KVER) CPU=$(CPU) modules

clean:
	rm -f *.o *.a *.pyc *.so
	rm -rf L865 L864 ppc 
	make -C $(KERNELSRC) M=`pwd` clean

install: mil1553.ko
	@echo "please specify one of {install-lab|install-oper|install-lhc|install-all}"
install-all: install-lab install-oper install-oplhc
install-lab: mil1553.ko
	dsc_install mil1553.ko /acc/dsc/lab/$(CPU)/$(KVER)/mil1553;
install-oper: mil1553.ko
	dsc_install mil1553.ko /acc/dsc/oper/$(CPU)/$(KVER)/mil1553;
install-oplhc: mil1553.ko
	dsc_install mil1553.ko /acc/dsc/oplhc/$(CPU)/$(KVER)/mil1553;
