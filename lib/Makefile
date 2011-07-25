MILIB = libmil1553
RTILIB = librti
QCKLIB = libquick

CPU=L865

include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I. -I../driver

MILSRC=$(MILIB).c $(MILIB).h $(RTILIB).c $(RTILIB).h $(QCKLIB).c $(QCKLIB).h

all: $(MILIB).$(CPU).a $(MILIB).$(CPU).so $(RTILIB).$(CPU).a $(RTILIB).$(CPU).so $(QCKLIB).$(CPU).a $(QCKLIB).$(CPU).so

$(MILIB).$(CPU).o: $(MILSRCS)

$(MILIB).$(CPU).a: $(MILIB).$(CPU).o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(MILIB).$(CPU).so: $(MILIB).$(CPU).o
	-$(RM) $@
	$(CC) $(CFLAGS) -o $@ -shared $^

$(RTILIB).$(CPU).o: $(MILSRCS)

$(RTILIB).$(CPU).a: $(RTILIB).$(CPU).o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(RTILIB).$(CPU).so: $(RTILIB).$(CPU).o
	-$(RM) $@
	$(CC) $(CFLAGS) -o $@ -shared $^

$(QCKLIB).$(CPU).o: $(MILSRCS)

$(QCKLIB).$(CPU).a: $(QCKLIB).$(CPU).o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(QCKLIB).$(CPU).so: $(QCKLIB).$(CPU).o
	-$(RM) $@
	$(CC) $(CFLAGS) -o $@ -shared $^

clean:
	-$(RM) $(MILIB).$(CPU).so $(MILIB).$(CPU).a $(MILIB).$(CPU).o
	-$(RM) $(RTILIB).$(CPU).so $(RTILIB).$(CPU).a $(RTILIB).$(CPU).o
	-$(RM) $(QCKLIB).$(CPU).so $(QCKLIB).$(CPU).a $(QCKLIB).$(CPU).o

install:
	dsc_install libmil1553.$(CPU).a /ps/local/$(CPU)/mil1553
	dsc_install librti.$(CPU).a /ps/local/$(CPU)/mil1553
	dsc_install libquick.$(CPU).a /ps/local/$(CPU)/mil1553
	dsc_install libmil1553.h /ps/local/$(CPU)/mil1553
	dsc_install librti.h /ps/local/$(CPU)/mil1553
	dsc_install libquick.h /ps/local/$(CPU)/mil1553
	dsc_install pow_messages.h /ps/local/$(CPU)/mil1553
