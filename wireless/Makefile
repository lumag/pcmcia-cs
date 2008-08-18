#
# Copyright (C) 1998 David A. Hinds -- dahinds@users.sourceforge.net
#
# Makefile 1.16 2003/01/12 18:03:20
#

# Include site dependent options and kernel configuration
include ../config.mk

CPPFLAGS += $(PCDEBUG) -D__KERNEL__ -DMODULE
CC = $(KCC) $(AFLAGS) $(KFLAGS)

MODULES = netwave_cs.o wavelan_cs.o ray_cs.o wvlan_cs.o airo_cs.o airo.o
SRCS    = netwave_cs.c wavelan_cs.c ray_cs.c airo_cs.c airo.c \
	  wvlan_cs.c wvlan_hcf.c wvlan_hcfio.c

ifdef DO_ORINOCO
MODULES += hermes.o orinoco.o orinoco_cs.o orinoco_plx.o
SRCS    += hermes.c orinoco.c orinoco_cs.c orinoco_plx.c
endif

all:	$(MODULES)

wvlan_cs.o: wvlan_base.o wvlan_hcf.o wvlan_hcfio.o
	$(LD) -r -o $@ $+ ; chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install: install-modules

include ../rules.mk
