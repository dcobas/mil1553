#================================================================
# Makefile to produce mil1553 test program
#================================================================

CPU=L865

include /acc/dsc/src/co/Make.auto

COMPILE_TIME:=$(shell date +%s)
GIT_VERSION = $(git describe --dirty --long)

CFLAGS = -g -Wall -I. -I../lib -I../driver
CFLAGS += -DCOMPILE_TIME=$(COMPILE_TIME)
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

LDLIBS= ../lib/libquick-serial.$(CPU).a

ALL  = mil1553test.$(CPU).o mil1553test.$(CPU)
ALL += decode.$(CPU) tdecode.$(CPU)

SRCS = mil1553test.c Mil1553Cmds.c DoCmd.c GetAtoms.c Cmds.c

HDRS = Cmds.h

all: $(ALL)

# Run on Workstation only

mil1553test.$(CPU).o: $(SRCS) $(HDRS)

mil1553test.$(CPU): mil1553test.$(CPU).o

decode.$(CPU): decode.$(CPU).o
tdecode.$(CPU): tdecode.$(CPU).o

clean:
	rm -f *.o *.$(CPU)

install: mil1553test.$(CPU) mil1553test.config MIL1553.regs
	@for f in $(ACCS); do \
	    dsc_install mil1553test.$(CPU) /acc/dsc/$$f/$(CPU)/mil1553; \
	    dsc_install mil1553test.config /acc/dsc/$$f/$(CPU)/mil1553; \
	    dsc_install MIL1553.regs /acc/dsc/$$f/$(CPU)/mil1553; \
	    dsc_install mil1553_news /acc/dsc/$$f/$(CPU)/mil1553; \
	done;

