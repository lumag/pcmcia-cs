#
# Copyright (C) 1998 David A. Hinds -- dhinds@pcmcia.sourceforge.org
#
# Makefile 1.3 2000/01/25 02:33:40
#

# Include site dependent options and kernel configuration
include ../config.mk

# Don't remove "-O3" or bad things will happen!
CFLAGS = -O3 -Wall -Wstrict-prototypes -pipe
CPPFLAGS += $(PCDEBUG) -D__KERNEL__ -DMODULE

CC += $(AFLAGS) $(KFLAGS)

MODULES = netwave_cs.o wavelan_cs.o ray_cs.o wvlan_cs.o airo_cs.o airo.o
SRCS    = netwave_cs.c wavelan_cs.c ray_cs.c airo_cs.c airo.c \
	  wvlan_cs.c wvlan_hcf.c wvlan_hcfio.c \

all:	$(MODULES)

wvlan_cs.o: wvlan_cs.c wvlan_hcf.o wvlan_hcfio.o
	$(CC) -c -MD $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv $*.d .depfiles
	$(LD) -r -o $@ .$@ wvlan_hcf.o wvlan_hcfio.o
	rm -f .$@ ; chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install: $(MODULES)
	mkdir -p $(PREFIX)/$(MODDIR)/pcmcia
	cp $(MODULES) $(PREFIX)/$(MODDIR)/pcmcia

include ../rules.mk
