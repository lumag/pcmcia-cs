#
# flash/Makefile 1.13 1999/10/28 01:22:05 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
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
