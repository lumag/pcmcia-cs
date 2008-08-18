#
# debug-tools/Makefile 1.31 1999/08/23 06:15:40 (David Hinds)
#

# Include site dependent options
include ../config.mk

ifndef CFLAGS
CFLAGS = -O -Wall -Wstrict-prototypes -pipe
XFLAGS = -O -pipe
endif
CPPFLAGS = -I../include -I$(LINUX)/include -I../modules
COFLAGS = -kv
YFLAGS = -d

CC := $(CC) $(UFLAGS)

SRCS = dump_cis.c dump_cisreg.c pack_cis.c
HDRS = pack_cis.h
TOOLS = dump_cis dump_cisreg pack_cis

ifdef CONFIG_PNP_BIOS
SRCS := $(SRCS) lspnp.c setpnp.c
TOOLS := $(TOOLS) lspnp setpnp
endif

ifdef CONFIG_ISA
SRCS := $(SRCS) dump_tcic.c dump_i365.c 
TOOLS := $(TOOLS) dump_tcic dump_i365
endif

all:	$(SRCS) $(HDRS) $(TOOLS)

dump_cis: dump_cis.o

dump_i365: dump_i365.o

dump_tcic: dump_tcic.o

dump_cisreg: dump_cisreg.o

lspnp: lspnp.o

setpnp: setpnp.o

yacc_cis.c yacc_cis.h: yacc_cis.y
	$(YACC) $(YFLAGS) yacc_cis.y
	mv y.tab.c yacc_cis.c
	mv y.tab.h yacc_cis.h

yacc_cis.o: yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c yacc_cis.c

lex_cis.o: lex_cis.c yacc_cis.h
	$(CC) $(XFLAGS) $(CPPFLAGS) -c lex_cis.c

pack_cis.o: pack_cis.c

pack_cis: pack_cis.o lex_cis.o yacc_cis.o
	$(CC) $(LFLAGS) -o pack_cis $+ -lm

parser: lex_cis.o yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -DDEBUG -o parser yacc_cis.c lex_cis.o

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f dump_tuples dump_i365 dump_tcic dump_cisreg

install: $(TOOLS)
	mkdir -p $(PREFIX)/sbin
	cp -f dump_cis pack_cis $(PREFIX)/sbin
ifdef CONFIG_PNP_BIOS
	cp -f lspnp setpnp $(PREFIX)/sbin
	mkdir -p $(PREFIX)/usr/share
	cp -f pnp.ids $(PREFIX)/usr/share
endif

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
