#
# Copyright (C) 1998 David A. Hinds -- dahinds@users.sourceforge.net
#
# Makefile 1.12 2001/08/27 03:14:13
#

# Include site dependent options and kernel configuration
include ../config.mk

# Don't remove "-O3" or bad things will happen!
CFLAGS = -O3 -Wall -Wstrict-prototypes -pipe
CPPFLAGS += $(PCDEBUG) -D__KERNEL__ -DMODULE
CC = $(KCC) $(AFLAGS) $(KFLAGS)

XFLAGS = $(CFLAGS) $(CPPFLAGS) $(MFLAG) -DEXPORT_SYMTAB

MODULES = netwave_cs.o wavelan_cs.o ray_cs.o wvlan_cs.o airo_cs.o airo.o
SRCS    = netwave_cs.c wavelan_cs.c ray_cs.c airo_cs.c airo.c \
	  wvlan_cs.c wvlan_hcf.c wvlan_hcfio.c

ifdef DO_ORINOCO
MODULES += hermes.o orinoco.o orinoco_cs.o
SRCS    += hermes.c orinoco.c orinoco_cs.c
endif

all:	$(MODULES)

wvlan_hcf.o wvlan_hcfio.o hermes.o orinoco.o: %.o: %.c
	$(CC) -MD -c $(XFLAGS) $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

wvlan_cs.o: wvlan_cs.c wvlan_hcf.o wvlan_hcfio.o
	$(CC) -c -MD $(CFLAGS) $(CPPFLAGS) $<
	@mkdir -p .depfiles ; mv $*.d .depfiles
	$(LD) -r -o .$@ $@ wvlan_hcf.o wvlan_hcfio.o
	mv .$@ $@ ; chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install: install-modules

include ../rules.mk
