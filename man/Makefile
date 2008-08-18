#
# man/Makefile 1.35 1998/04/26 22:46:20 (David Hinds)
#

# Include site dependent options
include ../config.mk

COFLAGS = -kv
MANDIR = $(PREFIX)/usr/man

MAN1 = cardinfo.1
MAN4 = pcmcia_core.4 tcic.4 i82365.4 tc589_cs.4 pcnet_cs.4 dummy_cs.4 \
	serial_cs.4 ftl_cs.4 memory_cs.4 fmvj18x_cs.4 tc574_cs.4 \
	tc575_cb.4 netwave_cs.4 wavelan_cs.4 aha152x_cs.4 xirc2ps_cs.4
MAN5 = pcmcia.5 stab.5
MAN8 = cardmgr.8 cardctl.8 ftl_format.8 ftl_check.8 scsi_info.8 \
	ifport.8 ifuser.8 ide_info.8 pcinitrd.8

all: $(MAN1) $(MAN4) $(MAN5) $(MAN8)

dep:

clean:

install-man1: $(MAN1)
	mkdir -p $(MANDIR)/man1
	cp $(MAN1) $(MANDIR)/man1

install-man4: $(MAN1)
	mkdir -p $(MANDIR)/man4
	cp $(MAN4) $(MANDIR)/man4

install-man5: $(MAN1)
	mkdir -p $(MANDIR)/man5
	cp $(MAN5) $(MANDIR)/man5

install-man8: $(MAN1)
	mkdir -p $(MANDIR)/man8
	cp $(MAN8) $(MANDIR)/man8

install: install-man1 install-man4 install-man5 install-man8
