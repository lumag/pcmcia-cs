#
# modules/Makefile 1.78 1998/02/05 07:46:57 (David Hinds)
#

# Include site dependent options and kernel configuration
include ../config.mk

ifndef CFLAGS
# Don't remove "-O2" or bad things will happen!
CFLAGS = -O2 -Wall -Wstrict-prototypes -Winline -pipe
endif

CPPFLAGS = $(PCDEBUG) -D__KERNEL__ -DMODULE -I../include \
	   -I$(LINUX)/include -I$(LINUX)
COFLAGS = -kv

# For files in kernel source tree, so that we can override config flags
XFLAGS = -O2 -D__KERNEL__ -I../include -I$(LINUX)/include
ifdef CONFIG_MODVERSIONS
XFLAGS := $(XFLAGS) $(MFLAG)
endif

SRCS = \
	i82365.c tcic.c cs.c cistpl.c rsrc_mgr.c bulkmem.c ds.c \
	serial_cs.c memory_cs.c ftl_cs.c \
	sram_mtd.c iflash2_mtd.c iflash2+_mtd.c

MODULES = \
	i82365.o tcic.o pcmcia_core.o ds.o \
	serial_cs.o memory_cs.o ftl_cs.o \
	sram_mtd.o iflash2_mtd.o iflash2+_mtd.o

CORE = cs.o cistpl.o rsrc_mgr.o bulkmem.o

EXTRA =

ifdef CONFIG_CARDBUS
CORE := $(CORE) cardbus.o
MODULES := $(MODULES) cb_enabler.o
endif

ifdef CONFIG_INET
MODULES := ${MODULES} \
	pcnet_cs.o 3c589_cs.o nmclan_cs.o fmvj18x_cs.o smc91c92_cs.o \
	netwave_cs.o wavelan_cs.o xirc2ps_cs.o 3c574_cs.o
SRCS := ${SRCS} \
	pcnet_cs.c 3c589_cs.c nmclan_cs.c fmvj18x_cs.c smc91c92_cs.c \
	netwave_cs.c wavelan_cs.c xirc2ps_cs.c 3c574_cs.c
EXTRA := ${EXTRA} 8390.o
ifdef CONFIG_TR
ifdef DO_IBMTR
MODULES := ${MODULES} ibmtr_cs.o
SRCS := ${SRCS} ibmtr_cs.c
endif
endif
endif

ifdef DO_IDE
MODULES := ${MODULES} fixed_cs.o
SRCS := ${SRCS} fixed_cs.c
endif

ifdef DO_AZTCD
MODULES := ${MODULES} aztcd_cs.o
EXTRA := ${EXTRA} aztcd.o
SRCS := ${SRCS} aztcd_cs.c
endif

ifdef CONFIG_SCSI
MODULES := ${MODULES} qlogic_cs.o aha152x_cs.o
SRCS := ${SRCS} qlogic_cs.c aha152x_cs.c
ifdef DO_FDOMAIN
MODULES := ${MODULES} fdomain_cs.o
SRCS := ${SRCS} fdomain_cs.c
endif
ifdef DO_T460
MODULES := ${MODULES} t460_cs.o
SRCS := ${SRCS} t460_cs.c
endif
endif

all:	$(MODULES) $(EXTRA)

8390.o: $(LINUX)/drivers/net/8390.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DMODULE $<

ibmtr.o: $(LINUX)/drivers/net/ibmtr.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<

ibmtr_cs.o: ibmtr_cs.c ibmtr.o
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	$(LD) -r -o $@ .$@ ibmtr.o
	rm -f .$@
	chmod -x $@

aztcd.o: $(LINUX)/drivers/cdrom/aztcd.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DMODULE $<

ifdef NEW_QLOGIC
qlogic.o: $(LINUX)/drivers/scsi/qlogicfas.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $< -o $@
else
qlogic.o: $(LINUX)/drivers/scsi/qlogic.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
endif

qlogic_cs.o: qlogic_cs.c qlogic.o
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	$(LD) -r -o $@ .$@ qlogic.o
	rm -f .$@
	chmod -x $@

3c59x_cb.o: 3c59x.c
	$(CC) -c $(XFLAGS) -DMODULE -DCARDBUS -o $@ $<

aha152x.o: $(LINUX)/drivers/scsi/aha152x.c ../include/linux/config.h
ifdef FIX_AHA152X
	patch $< patches/aha152x.fix -o aha152x.c
	$(CC) -c $(XFLAGS) -I$(LINUX)/drivers/scsi \
		-DPCMCIA -D__NO_VERSION__ aha152x.c
	rm aha152x.c
else
	$(CC) -c $(XFLAGS) $(AHAFLAG) -DPCMCIA -D__NO_VERSION__ $<
endif

aha152x_cs.o: aha152x_cs.c aha152x.o
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	$(LD) -r -o $@ .$@ aha152x.o
	rm -f .$@
	chmod -x $@

fdomain.o: $(LINUX)/drivers/scsi/fdomain.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<

fdomain_cs.o: fdomain_cs.c fdomain.o
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	$(LD) -r -o $@ .$@ fdomain.o
	rm -f .$@
	chmod -x $@

pcmcia_core.o: $(CORE)
	$(LD) -r -o pcmcia_core.o $(CORE)
	chmod -x pcmcia_core.o

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

MODDIR = $(PREFIX)/lib/modules/$(UTS_RELEASE)

install-modules: $(MODULES)
	mkdir -p $(MODDIR)/pcmcia
	cp -p $(MODULES) $(MODDIR)/pcmcia

install-extra: $(EXTRA)
	if [ -f 8390.o ] ; then				\
	    mkdir -p $(MODDIR)/net ;			\
	    cp -p 8390.o $(MODDIR)/net ;		\
	fi
	if [ -f aztcd.o ] ; then			\
	    mkdir -p $(MODDIR)/cdrom ;			\
	    cp -p aztcd.o $(MODDIR)/cdrom ;		\
	fi

install: install-modules install-extra

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $<

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
