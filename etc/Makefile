#
# etc/Makefile 1.56 2000/04/27 02:07:17 (David Hinds)
#

# Include site dependent options
include ../config.mk

all:
ifeq ($(HOST_ARCH),$(ARCH))
	@set -e ; $(MAKE) -C cis
endif

dep:
clean:

install:
	@set -e ; $(MAKE) -C cis install
	@./install-etc
