MILIB = libmil1553
RTILIB = librti
QCKLIB = libquick

CPU=L865

include /acc/dsc/src/co/Make.auto

.o.a:
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

CFLAGS= -g -Wall -I. -I../driver -fPIC
ARFLAGS=rcv

MILSRC=$(MILIB).c $(MILIB).h $(RTILIB).c $(RTILIB).h $(QCKLIB).c $(QCKLIB).h

all: $(MILIB).$(CPU).a $(RTILIB).$(CPU).a $(QCKLIB).$(CPU).a

$(MILIB).$(CPU).o: $(MILSRCS)
$(RTILIB).$(CPU).o: $(MILSRCS)
$(QCKLIB).$(CPU).o: $(MILSRCS)

clean:
	-$(RM) $(MILIB).$(CPU).so $(MILIB).$(CPU).a $(MILIB).$(CPU).o
	-$(RM) $(RTILIB).$(CPU).so $(RTILIB).$(CPU).a $(RTILIB).$(CPU).o
	-$(RM) $(QCKLIB).$(CPU).so $(QCKLIB).$(CPU).a $(QCKLIB).$(CPU).o
	-$(RM) -rf html Doxyfile

install:
	dsc_install libmil1553.$(CPU).a /acc/local/$(CPU)/mil1553
	dsc_install librti.$(CPU).a /acc/local/$(CPU)/mil1553
	dsc_install libquick.$(CPU).a /acc/local/$(CPU)/mil1553
	dsc_install libmil1553.h /acc/local/$(CPU)/mil1553
	dsc_install librti.h /acc/local/$(CPU)/mil1553
	dsc_install libquick.h /acc/local/$(CPU)/mil1553
	dsc_install pow_messages.h /acc/local/$(CPU)/mil1553
	dsc_install ../driver/mil1553.h /acc/local/$(CPU)/mil1553

docs: Doxyfile.patch
	doxygen -s -g
	patch -p0 < Doxyfile.patch
	doxygen
	rm Doxyfile
