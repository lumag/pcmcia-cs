#
# Copyright (C) 1998 David A. Hinds -- dhinds@hyper.stanford.edu
#
# Makefile 1.101 1999/10/19 21:22:37
#

# Include site dependent options and kernel configuration
include ../config.mk

ifndef CFLAGS
# Don't remove "-O3" or bad things will happen!
CFLAGS = -O3 -Wall -Wstrict-prototypes -pipe
endif

CC := $(CC) $(AFLAGS) $(KFLAGS)

CPPFLAGS = $(PCDEBUG) -D__KERNEL__ -DMODULE -I../include -I$(LINUX)/include
COFLAGS = -kv

SRCS = i82365.c tcic.c cs.c cistpl.c rsrc_mgr.c bulkmem.c ds.c

MODULES = i82365.o tcic.o pcmcia_core.o ds.o

CORE = cs.o cistpl.o rsrc_mgr.o bulkmem.o

ifdef CONFIG_CARDBUS
SRCS := $(SRCS) cardbus.c cb_enabler.c
CORE := $(CORE) cardbus.o
MODULES := $(MODULES) cb_enabler.o
endif

ifdef CONFIG_PNP_BIOS
SRCS := $(SRCS) pnp_bios.c pnp_proc.c pnp_rsrc.c
CORE := $(CORE) pnp_bios.o pnp_proc.o pnp_rsrc.o
endif

all:	$(MODULES)

pcmcia_core.o: $(CORE)
	$(LD) -r -o pcmcia_core.o $(CORE)
	chmod -x pcmcia_core.o

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install install-modules: $(MODULES)
	mkdir -p $(PREFIX)/$(MODDIR)/pcmcia
	cp $(MODULES) $(PREFIX)/$(MODDIR)/pcmcia

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $<

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
