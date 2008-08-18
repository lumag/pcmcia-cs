#
# Copyright (C) 1998 David A. Hinds -- dahinds@users.sourceforge.net
#
# Makefile 1.129 2000/06/15 02:15:27
#

# Include site dependent options and kernel configuration
include ../config.mk

# For files in kernel source tree, so that we can override config flags
XFLAGS := -O2 $(CPPFLAGS) -D__KERNEL__ -DEXPORT_SYMTAB $(MFLAG)

# Don't remove "-O3" or bad things will happen!
CFLAGS = -O3 -Wall -Wstrict-prototypes -pipe
CPPFLAGS += $(PCDEBUG) -D__KERNEL__ -DMODULE

CC += $(AFLAGS) $(KFLAGS)

SRCS    = serial_cs.c memory_cs.c ftl_cs.c dummy_cs.c \
	  sram_mtd.c iflash2_mtd.c iflash2+_mtd.c
MODULES = serial_cs.o memory_cs.o ftl_cs.o dummy_cs.o \
	  sram_mtd.o iflash2_mtd.o iflash2+_mtd.o
EXTRA   =
I_EXTRA =

ifdef CONFIG_CARDBUS
SRCS    += memory_cb.c serial_cb.c
MODULES += memory_cb.o serial_cb.o
ifdef CONFIG_INET
SRCS    += 3c575_cb.c tulip_cb.c epic_cb.c eepro100_cb.c
MODULES += 3c575_cb.o tulip_cb.o epic_cb.o eepro100_cb.o
endif
ifdef CONFIG_SCSI
ifdef DO_APA1480
SRCS    += apa1480_cb.c aic7xxx.c
MODULES += apa1480_cb.o
endif
endif
endif

ifdef CONFIG_INET
MODULES += pcnet_cs.o 3c589_cs.o nmclan_cs.o fmvj18x_cs.o smc91c92_cs.o \
	   xirc2ps_cs.o 3c574_cs.o
SRCS    += pcnet_cs.c 3c589_cs.c nmclan_cs.c fmvj18x_cs.c smc91c92_cs.c \
	   xirc2ps_cs.c 3c574_cs.c
SRCS    += 8390.c
EXTRA   += 8390.o
I_EXTRA += install-8390
ifdef CONFIG_TR
MODULES += ibmtr_cs.o
SRCS    += ibmtr_cs.c
endif
endif

ifdef DO_IDE
SRCS    += ide_cs.c
MODULES += ide_cs.o
endif

ifdef DO_PARPORT
SRCS    += parport_cs.c
MODULES += parport_cs.o
endif

vpath %.c $(LINUX)/drivers/net $(LINUX)/drivers/scsi \
	$(LINUX)/drivers/net/tokenring

SCSI=$(LINUX)/drivers/scsi

ifdef CONFIG_SCSI
SRCS    += qlogicfas.c aha152x.c fdomain.c
SRCS    += qlogic_stub.c aha152x_stub.c fdomain_stub.c
MODULES += qlogic_cs.o aha152x_cs.o fdomain_cs.o
endif

all:	$(MODULES) $(EXTRA)

8390.o: 8390.c
	$(CC) -MD -c $(XFLAGS) -DMODULE $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

ibmtr.o: ibmtr.c
	$(CC) -MD -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

ibmtr_cs.o: ibmtr_cs.c ibmtr.o
	$(CC) -c -MD $(CFLAGS) $(CPPFLAGS) $< -o .$@
	@mkdir -p .depfiles ; mv $*.d .depfiles
	$(LD) -r -o $@ .$@ ibmtr.o
	rm -f .$@ ; chmod -x $@

qlogicfas.o aha152x.o fdomain.o: %.o: %.c
	$(CC) -MD -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

qlogic_cs.o: qlogic_stub.o qlogicfas.o
	$(LD) -r -o $@ $+ ; chmod -x $@

aha152x_cs.o: aha152x_stub.o aha152x.o
	$(LD) -r -o $@ $+ ; chmod -x $@

fdomain_cs.o: fdomain_stub.o fdomain.o
	$(LD) -r -o $@ $+ ; chmod -x $@

3c575_cb.o tulip_cb.o epic_cb.o eepro100_cb.o: %.o: %.c
	$(CC) -MD -c $(XFLAGS) -DMODULE -DCARDBUS $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

aic7xxx.o: aic7xxx.c
ifdef FIX_AIC7XXX
	cd $(SCSI) ; if [ -r aic7xxx_asm.c -a ! -r aic7xxx_seq.h ] ; \
		then $(CC) -o aic7xxx_asm aic7xxx_asm.c ; \
		./aic7xxx_asm -o aic7xxx_seq.h aic7xxx.seq ; fi
	patch -s $< patches/aic7xxx.old -o aic7xxx.c 2>/dev/null || \
		( rm -f aic7xxx.c aic7xxx.c.rej aic7xxx.c.orig && \
		patch -s $< patches/aic7xxx.fix -o aic7xxx.c )
	$(CC) -MD -c $(CFLAGS) $(CPPFLAGS) -I$(SCSI) aic7xxx.c
	rm aic7xxx.c
else
	$(CC) -MD -c $(XFLAGS) -DPCMCIA -D__NO_VERSION__ $<
endif
	@mkdir -p .depfiles ; mv $*.d .depfiles

apa1480_cb.o: apa1480_stub.o aic7xxx.o
	$(LD) -r -o $@ $+ ; chmod -x $@

clean:
	rm -f core core.* *.o .*.o *.s *.a *~ .depend .depfiles/*.d

install-modules: $(MODULES)
	@mkdir -p $(PREFIX)$(MODDIR)/pcmcia
	cp $(MODULES) $(PREFIX)$(MODDIR)/pcmcia

install-8390: 8390.o
	@mkdir -p $(PREFIX)$(MODDIR)/net
	cp 8390.o $(PREFIX)$(MODDIR)/net

install: install-modules $(I_EXTRA)

include ../rules.mk
