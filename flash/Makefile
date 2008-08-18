#
# flash/Makefile 1.9 1997/09/16 07:00:11 (David Hinds)
#

# Include site dependent options
include ../config.mk

#ifndef CFLAGS
CFLAGS = -O -Wall -Wstrict-prototypes -pipe -I../include
XFLAGS = -O -pipe -I../include
#endif

COFLAGS = -kv

SRCS = ftl_format.c ftl_check.c
TOOLS = ftl_format ftl_check

all:	$(TOOLS)

ftl_format: ftl_format.o

ftl_check: ftl_check.o

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS)

install: $(TOOLS)
	mkdir -p $(PREFIX)/sbin
	cp -f $(TOOLS) $(PREFIX)/sbin

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
