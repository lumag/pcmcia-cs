#
# man/Makefile 1.40 1999/07/23 20:06:23 (David Hinds)
#

# Include site dependent options
include ../config.mk

COFLAGS = -kv
MANDIR = $(PREFIX)/usr/man

MAN1_X11 = cardinfo.1
MAN4 = pcmcia_core.4 tcic.4 i82365.4 tc589_cs.4 pcnet_cs.4 dummy_cs.4 \
	serial_cs.4 ftl_cs.4 memory_cs.4 fmvj18x_cs.4 tc574_cs.4 \
	tc575_cb.4 netwave_cs.4 wavelan_cs.4 aha152x_cs.4 xirc2ps_cs.4 \
	memory_cb.4 ibmtr_cs.4 smc91c92_cs.4
MAN5 = pcmcia.5 stab.5
MAN8 = cardmgr.8 cardctl.8 ftl_format.8 ftl_check.8 scsi_info.8 \
	ifport.8 ifuser.8 ide_info.8 pcinitrd.8 pack_cis.8 dump_cis.8

all: $(MAN1) $(MAN4) $(MAN5) $(MAN8)

dep:

clean:

install-man1-x11: $(MAN1_X11)
	mkdir -p $(PREFIX)/usr/X11R6/man/man1
	cp $(MAN1_X11) $(PREFIX)/usr/X11R6/man/man1

install-man4: $(MAN4)
	mkdir -p $(MANDIR)/man4
	cp $(MAN4) $(MANDIR)/man4

install-man5: $(MAN5)
	mkdir -p $(MANDIR)/man5
	cp $(MAN5) $(MANDIR)/man5

install-man8: $(MAN8)
	mkdir -p $(MANDIR)/man8
	cp $(MAN8) $(MANDIR)/man8

install: install-man1-x11 install-man4 install-man5 install-man8
