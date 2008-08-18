#
# Copyright (C) 1998 David A. Hinds -- dhinds@pcmcia.sourceforge.org
#
# Makefile 1.110 1999/11/01 18:18:58
#

# Include site dependent options and kernel configuration
include ../config.mk

# Don't remove "-O3" or bad things will happen!
CFLAGS = -O3 -Wall -Wstrict-prototypes -pipe
CPPFLAGS = $(PCDEBUG) -D__KERNEL__ -DMODULE -I../include -I$(LINUX)/include
COFLAGS = -kv

CC := $(CC) $(AFLAGS) $(KFLAGS)

# For files in kernel source tree, so that we can override config flags
XFLAGS = -O2 -D__KERNEL__ -I../include -I$(LINUX)/include -DEXPORT_SYMTAB
ifdef CONFIG_MODVERSIONS
XFLAGS := $(XFLAGS) $(MFLAG)
endif

SRCS = \
	serial_cs.c memory_cs.c ftl_cs.c dummy_cs.c \
	sram_mtd.c iflash2_mtd.c iflash2+_mtd.c

MODULES = \
	serial_cs.o memory_cs.o ftl_cs.o dummy_cs.o \
	sram_mtd.o iflash2_mtd.o iflash2+_mtd.o

EXTRA =

ifdef CONFIG_CARDBUS
SRCS := $(SRCS) 3c575_cb.c tulip_cb.c memory_cb.c serial_cb.c
MODULES := $(MODULES) 3c575_cb.o tulip_cb.o memory_cb.o serial_cb.o
ifdef CONFIG_SCSI
ifdef DO_APA1480
MODULES := ${MODULES} apa1480_cb.o
SRCS := ${SRCS} apa1480_cb.c
endif
endif
ifdef DO_EPIC_CB
MODULES := ${MODULES} epic_cb.o
endif
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
MODULES := ${MODULES} ibmtr_cs.o
SRCS := ${SRCS} ibmtr_cs.c
endif
endif

ifdef DO_IDE
MODULES := ${MODULES} ide_cs.o
SRCS := ${SRCS} ide_cs.c
endif

ifdef DO_PARPORT
MODULES := ${MODULES} parport_cs.o
SRCS := ${SRCS} parport_cs.c
endif

SCSI=$(LINUX)/drivers/scsi

ifdef CONFIG_SCSI
MODULES := ${MODULES} qlogic_cs.o aha152x_cs.o fdomain_cs.o
SRCS := ${SRCS} qlogic_cs.c aha152x_cs.c fdomain_cs.c
endif

all:	$(MODULES) $(EXTRA)

8390.o: $(LINUX)/drivers/net/8390.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DMODULE $<

ibmtr.o: $(LINUX)/drivers/net/ibmtr.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<

ibmtr_cs.o: ibmtr_cs.c ibmtr.o
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv ibmtr_cs.d .depfiles
	$(LD) -r -o $@ .$@ ibmtr.o
	rm -f .$@
	chmod -x $@

ifdef NEW_QLOGIC
qlogic.o: $(SCSI)/qlogicfas.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $< -o $@
else
qlogic.o: $(SCSI)/qlogic.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
endif

qlogic_cs.o: qlogic_cs.c qlogic.o
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv qlogic_cs.d .depfiles
	$(LD) -r -o $@ .$@ qlogic.o
	rm -f .$@
	chmod -x $@

3c575_cb.o: 3c575_cb.c
	$(CC) -MD -c $(XFLAGS) -DMODULE -DCARDBUS -o $@ $<
	@mkdir -p .depfiles ; mv 3c575_cb.d .depfiles

tulip_cb.o: tulip_cb.c
	$(CC) -MD -c $(XFLAGS) -DMODULE -DCARDBUS -o $@ $<
	@mkdir -p .depfiles ; mv tulip_cb.d .depfiles

epic_cb.o: $(LINUX)/drivers/net/epic100.c
	$(CC) -MD -c $(XFLAGS) -DMODULE -DCARDBUS -o $@ $<
	@mkdir -p .depfiles ; mv epic100.d .depfiles

aha152x.o: $(SCSI)/aha152x.c ../include/linux/config.h
ifdef FIX_AHA152X
	patch -s $< patches/aha152x.fix -o aha152x.c
	$(CC) -c $(XFLAGS) -I$(SCSI) -DPCMCIA -D__NO_VERSION__ aha152x.c
	rm aha152x.c
else
	$(CC) -c $(XFLAGS) $(AHAFLAG) -DPCMCIA -D__NO_VERSION__ $<
endif

aha152x_cs.o: aha152x_cs.c aha152x.o
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv aha152x_cs.d .depfiles
	$(LD) -r -o $@ .$@ aha152x.o
	rm -f .$@ ; chmod -x $@

fdomain.o: $(SCSI)/fdomain.c ../include/linux/config.h
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<

fdomain_cs.o: fdomain_cs.c fdomain.o
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv fdomain_cs.d .depfiles
	$(LD) -r -o $@ .$@ fdomain.o
	rm -f .$@ ; chmod -x $@

aic7xxx.o: $(SCSI)/aic7xxx.c
ifdef FIX_AIC7XXX
	cd $(SCSI) ; if [ -r aic7xxx_asm.c -a ! -r aic7xxx_seq.h ] ; \
		then $(CC) -o aic7xxx_asm aic7xxx_asm.c ; \
		./aic7xxx_asm -o aic7xxx_seq.h aic7xxx.seq ; fi
	patch -s $< patches/aic7xxx.old -o aic7xxx.c 2>/dev/null || \
		( rm -f aic7xxx.c aic7xxx.c.rej aic7xxx.c.orig && \
		patch -s $< patches/aic7xxx.fix -o aic7xxx.c )
	$(CC) -c $(XFLAGS) -I$(SCSI) -DPCMCIA -D__NO_VERSION__ aic7xxx.c
	rm aic7xxx.c
else
	$(CC) -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
endif

apa1480_cb.o: apa1480_cb.c aic7xxx.o
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv apa1480_cb.d .depfiles
	$(LD) -r -o $@ .$@ aic7xxx.o
	rm -f .$@ ; chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install-modules: $(MODULES)
	mkdir -p $(PREFIX)/$(MODDIR)/pcmcia
	cp $(MODULES) $(PREFIX)/$(MODDIR)/pcmcia

install-extra: $(EXTRA)
	if [ -f 8390.o ] ; then				\
	    mkdir -p $(PREFIX)/$(MODDIR)/net ;		\
	    cp 8390.o $(PREFIX)/$(MODDIR)/net ;		\
	fi

install: install-modules install-extra

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $<

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
