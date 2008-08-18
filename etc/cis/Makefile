#
# etc/cis/Makefile $Version$ 1998/04/06 23:05:25 (David Hinds)
#

# Include site dependent options
include ../../config.mk

ETC = $(PREFIX)/etc/pcmcia
PACK = ../../debug-tools/pack_cis

CIS = $(patsubst %.cis,%.dat,$(wildcard *.cis))

all: $(CIS)

dep:

clean:
	rm -f *.dat

install: $(CIS)
	mkdir -p $(ETC)/cis
	cp *.dat $(ETC)/cis

.SUFFIXES: .dat .cis
.cis.dat:
	$(PACK) -o $@ $<
