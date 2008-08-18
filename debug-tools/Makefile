#
# debug-tools/Makefile 1.23 1998/04/02 08:26:15 (David Hinds)
#

# Include site dependent options
include ../config.mk

ifndef CFLAGS
CFLAGS = -O -Wall -Wstrict-prototypes -pipe
XFLAGS = -O -pipe
endif
CPPFLAGS = -I../include -I../modules

COFLAGS = -kv
YFLAGS = -d

SRCS = dump_tuples.c dump_cisreg.c pack_cis.c
HDRS = pack_cis.h
TOOLS = dump_tuples dump_cisreg pack_cis

ifdef CONFIG_ISA
SRCS := $(SRCS) dump_tcic.c dump_i365.c
TOOLS := $(TOOLS) dump_tcic dump_i365
endif

all:	$(SRCS) $(HDRS) $(TOOLS)

dump_tuples: dump_tuples.o

dump_i365: dump_i365.o

dump_tcic: dump_tcic.o

dump_cisreg: dump_cisreg.o

yacc_cis.o: yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c yacc_cis.c

lex_cis.o: lex_cis.c yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c lex_cis.c

pack_cis.o: pack_cis.c

pack_cis: pack_cis.o lex_cis.o yacc_cis.o
	$(CC) $(LFLAGS) -o pack_cis $+ -lm

parser: lex_cis.o yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -DDEBUG -o parser yacc_cis.c lex_cis.o

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f dump_tuples dump_i365 dump_tcic dump_cisreg

install:

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
