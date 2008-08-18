#
# debug-tools/Makefile 1.21 1997/09/16 06:59:01 (David Hinds)
#

# Include site dependent options
include ../config.mk

ifndef CFLAGS
CFLAGS = -O -Wall -Wstrict-prototypes -pipe
endif
CPPFLAGS = -I../include -I../modules

COFLAGS = -kv

SRCS = dump_tuples.c dump_i365.c dump_tcic.c dump_cisreg.c

all:	dump_tuples dump_i365 dump_tcic dump_cisreg

dump_tuples: dump_tuples.o

dump_i365: dump_i365.o

dump_tcic: dump_tcic.o

dump_cisreg: dump_cisreg.o

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f dump_tuples dump_i365 dump_tcic dump_cisreg

install:

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
