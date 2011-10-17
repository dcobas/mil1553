CPU=L865

include /acc/dsc/src/co/Make.auto

CFLAGS= -g -I. -I../../driver -I../../lib

OBJS= tst_csr.$(CPU).o csr.$(CPU).o print.$(CPU).o \
      bounce.$(CPU).o broadcst.$(CPU).o buffer.$(CPU).o \
      mode_code.$(CPU).o mode_data.$(CPU).o \
      procedures.$(CPU).o choix_rt.$(CPU).o \
      signat.$(CPU).o 1553def.$(CPU).o

ALL: libtest.$(CPU).a mdaccept.$(CPU)

.o: $*.c
	$(CC) $(CFLAGS) -c $*.c


libtest.$(CPU).a: $(OBJS)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@


mdaccept.$(CPU): main.$(CPU).o libtest.$(CPU).a ../../lib/libmil1553.$(CPU).o ../../lib/librti.$(CPU).o
	$(CC) -o mdaccept.$(CPU) main.$(CPU).o -L. -ltest.$(CPU) ../../lib/libmil1553.$(CPU).o ../../lib/librti.$(CPU).o


clean:
	rm -f *.$(CPU).o
	rm -f libtest.$(CPU).a
