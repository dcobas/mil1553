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

install:
	@echo "please specify one of {install-lab|install-oper|install-lhc|install-all}"

install-all: install-lab install-oper install-oplhc

install-lab: install_mil1553.sh mil1553.awk
	dsc_install install_mil1553.sh /acc/dsc/lab/$(CPU)/$(KVER)/mil1553;
	dsc_install mil1553.awk /acc/dsc/lab/$(CPU)/$(KVER)/mil1553;

install-oper: install_mil1553.sh mil1553.awk
	dsc_install install_mil1553.sh /acc/dsc/oper/$(CPU)/$(KVER)/mil1553;
	dsc_install mil1553.awk /acc/dsc/oper/$(CPU)/$(KVER)/mil1553;

install-oplhc: install_mil1553.sh mil1553.awk
	dsc_install install_mil1553.sh /acc/dsc/oplhc/$(CPU)/$(KVER)/mil1553;
	dsc_install mil1553.awk /acc/dsc/oplhc/$(CPU)/$(KVER)/mil1553;

