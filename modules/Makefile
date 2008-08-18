#
# Copyright (C) 1998 David A. Hinds -- dhinds@hyper.stanford.edu
#
# Makefile 1.98 1999/07/30 04:11:39
#

# Include site dependent options and kernel configuration
include ../config.mk

ifndef CFLAGS
# Don't remove "-O2" or bad things will happen!
CFLAGS = -O2 -Wall -Wstrict-prototypes -pipe
endif

CC := $(CC) $(AFLAGS) $(KFLAGS)

CPPFLAGS = $(PCDEBUG) -D__KERNEL__ -DMODULE -I../include \
	   -I$(LINUX)/include -I$(LINUX)
COFLAGS = -kv

# For files in kernel source tree, so that we can override config flags
XFLAGS = -O2 -D__KERNEL__ -I../include -I$(LINUX)/include
ifdef CONFIG_MODVERSIONS
XFLAGS := $(XFLAGS) $(MFLAG)
endif

SRCS = i82365.c tcic.c cs.c cistpl.c rsrc_mgr.c bulkmem.c ds.c

MODULES = i82365.o tcic.o pcmcia_core.o ds.o

CORE = cs.o cistpl.o rsrc_mgr.o bulkmem.o

ifdef CONFIG_CARDBUS
SRCS := $(SRCS) cardbus.c cb_enabler.c
CORE := $(CORE) cardbus.o
MODULES := $(MODULES) cb_enabler.o
endif

all:	$(MODULES)

pcmcia_core.o: $(CORE)
	$(LD) -r -o pcmcia_core.o $(CORE)
	chmod -x pcmcia_core.o

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install install-modules: $(MODULES)
	mkdir -p $(PREFIX)/$(MODDIR)/pcmcia
	cp -p $(MODULES) $(PREFIX)/$(MODDIR)/pcmcia

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $<

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
