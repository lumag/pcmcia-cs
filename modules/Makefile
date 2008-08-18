#
# Copyright (C) 1998 David A. Hinds -- dhinds@pcmcia.sourceforge.org
#
# Makefile 1.111 2000/01/24 06:51:44
#

# Include site dependent options and kernel configuration
include ../config.mk

# Don't remove "-O2" or bad things will happen!  We use -O2 rather
# than -O3 because it yields smaller code; this is not performance
# critical stuff.

CFLAGS = -O2 -Wall -Wstrict-prototypes -pipe
CPPFLAGS += $(PCDEBUG) -D__KERNEL__ -DMODULE

CC += $(AFLAGS) $(KFLAGS)

SRCS    = i82365.c tcic.c cs.c cistpl.c rsrc_mgr.c bulkmem.c ds.c
CORE    = cs.o cistpl.o rsrc_mgr.o bulkmem.o
MODULES = i82365.o tcic.o pcmcia_core.o ds.o

ifdef CONFIG_CARDBUS
SRCS    += cardbus.c cb_enabler.c
CORE    += cardbus.o
MODULES += cb_enabler.o
endif

ifeq ($(ARCH),i386)
ifdef CONFIG_PCI
SRCS    += pci_pirq.c
CORE    += pci_pirq.o
endif
endif

ifdef CONFIG_PNP_BIOS
SRCS    += pnp_bios.c pnp_proc.c pnp_rsrc.c
CORE    += pnp_bios.o pnp_proc.o pnp_rsrc.o
endif

all:	$(MODULES)

pcmcia_core.o: $(CORE)
	$(LD) -r -o $@ $(CORE)
	chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install install-modules: $(MODULES)
	mkdir -p $(PREFIX)/$(MODDIR)/pcmcia
	cp $(MODULES) $(PREFIX)/$(MODDIR)/pcmcia

include ../rules.mk
