#
# flash/Makefile 1.12 1999/07/24 15:03:39 (David Hinds)
#

# Include site dependent options
include ../config.mk

#ifndef CFLAGS
CFLAGS = -O -Wall -Wstrict-prototypes -pipe
#endif
CPPFLAGS = -I../include -I$(LINUX)/include
COFLAGS = -kv

CC := $(CC) $(UFLAGS)

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
