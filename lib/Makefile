MILIB = libmil1553
RTILIB = librti
QCKLIB = libquick
TSTLIB = libquick-serial

CPU=L865

include /acc/dsc/src/co/Make.auto

%.$(CPU).a: %.$(CPU).o
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

CFLAGS= -g -Wall -I. -I../driver -fPIC
ARFLAGS=rcv

MILSRC=$(MILIB).c $(MILIB).h $(RTILIB).c $(RTILIB).h $(QCKLIB).c \
	$(QCKLIB).h $(TSTLIB).c $(TSTLIB).h

all: $(MILIB).$(CPU).a $(RTILIB).$(CPU).a \
	$(QCKLIB).$(CPU).a $(TSTLIB).$(CPU).a

$(MILIB).$(CPU).o: $(MILSRCS)
$(RTILIB).$(CPU).o: $(MILSRCS)
$(QCKLIB).$(CPU).o: $(MILSRCS)
$(TSTLIB).$(CPU).o: $(MILSRCS)

clean:
	-$(RM) -rf *.$(CPU).a *.$(CPU).o 
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
