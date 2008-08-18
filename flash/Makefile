#
# flash/Makefile 1.17 2001/01/18 03:15:06 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
CC := $(CC:kgcc=gcc) $(UFLAGS)

SRCS  = ftl_format.c ftl_check.c
TOOLS = ftl_format ftl_check

all:	$(TOOLS)

ftl_format: ftl_format.o

ftl_check: ftl_check.o

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS)

install: $(TOOLS)
	@mkdir -p $(PREFIX)/sbin
	cp -f $(TOOLS) $(PREFIX)/sbin

include ../rules.mk
