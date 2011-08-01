#================================================================
# Makefile to produce mil1553 test program
#================================================================

CPU=L865

include /acc/dsc/src/co/Make.auto

COMPILE_TIME:=$(shell date +%s)

CFLAGS = -g -Wall -I. -I../lib -I../driver -DCOMPILE_TIME=$(COMPILE_TIME)

LDLIBS= ../lib/libmil1553.$(CPU).a ../lib/librti.$(CPU).a ../lib/libquick.$(CPU).a

ALL  = mil1553test.$(CPU).o mil1553test.$(CPU)

SRCS = mil1553test.c Mil1553Cmds.c DoCmd.c GetAtoms.c Cmds.c

HDRS = Cmds.h

all: $(ALL)

# Run on Workstation only

mil1553test.$(CPU).o: $(SRCS) $(HDRS)

mil1553test.$(CPU): mil1553test.$(CPU).o

clean:
	rm -f *.o *.$(CPU)

install: mil1553test.$(CPU) mil1553test.config MIL1553.regs
	@for f in $(ACCS); do \
	    dsc_install mil1553test.$(CPU) /acc/dsc/$$f/$(CPU)/mil1553; \
	    dsc_install mil1553test.config /acc/dsc/$$f/$(CPU)/mil1553; \
	    dsc_install MIL1553.regs /acc/dsc/$$f/$(CPU)/mil1553; \
	done;

