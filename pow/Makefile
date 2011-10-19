#========================================================
# Makefile for EM DSC & DSCPM and associated RT tasks
#========================================================

EM=POW
CFLAGS=-g -Wall
LDFLAGS=-g

PROCS=pci_procos.c
RTS=  pci-powvrt.c

# CFLAGS +=-I /acc/local/$(CPU)/mil1553
CFLAGS +=-I/acc/src/dsc/drivers/cohtdrep/lewis/new_mil1553/lib
CFLAGS +=-I/acc/src/dsc/drivers/cohtdrep/lewis/new_mil1553/driver

LDLIBS=-lgm -ldscrt $(TGMLIBS) -L/acc/src/dsc/drivers/cohtdrep/lewis/new_mil1553/lib -lquick.L865 -lmil1553.L865 -lrti.L865 -ldrvrutil  -lerr $(XTRALIBS)
LDFLAGS += -L /acc/local/L865/mil1553 -L/acc/src/dsc/drivers/cohtdrep/lewis/new_mil1553/lib
LDLIBS=-lgm -ldscrt $(TGMLIBS) -lquick.$(CPU) -lmil1553.$(CPU) -lrti.$(CPU) -ldrvrutil -lerr $(XTRALIBS)

RTX=

include /acc/src/dsc/co/em/Make.gen

clean:
	$(RM) $(LIB) $(PROCO) *.$(CPU).o $(RTE) $(RTO) $(BAKS)

powvrt.$(CPU).o: powrt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DV_VERSION -c $< -o $@
	
powvrt-ade.$(CPU).o: powrt-ade.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DV_VERSION -c $< -o $@	

powvrt-cngs.$(CPU).o: powrt-cngs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DV_VERSION -c $< -o $@
	
mg64t.$(CPU).o: mg64t.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@		
	
powvrt.$(CPU).o: pow_messages.h

pow-v_procos.$(CPU).o: pow_messages.h

powvrt-pci.$(CPU).o: powvrt-pci.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

powvrt-pci.$(CPU): powvrt-pci.$(CPU).o 
	$(LINK.o) $^  $(LOADLIBES) $(LDFLAGS) $(LDLIBS) -o $@
