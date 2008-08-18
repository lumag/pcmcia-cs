#
# cardmgr/Makefile 1.82 2000/11/07 19:00:35 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
CPPFLAGS += -I../modules

CC += $(UFLAGS)

SRCS  = cardmgr.c cardctl.c yacc_config.c lex_config.c probe.c
TOOLS = cardmgr cardctl

ifdef CONFIG_INET
SRCS  += ifport.c ifuser.c
TOOLS += ifport ifuser
endif

ifdef CONFIG_SCSI
SRCS  += scsi_info.c
TOOLS += scsi_info
endif

ifdef DO_IDE
SRCS  += ide_info.c
TOOLS += ide_info
endif

ifdef HAS_FORMS
SRCS  += cardinfo.c
CPPFLAGS += -I/usr/X11R6/include -I/usr/X11/include -I/usr/X11R6/include/X11
EXTRA += cardinfo
I_EXTRA += install-cardinfo
endif

ifdef CONFIG_ISA
PROBE = probe
ifdef PREFIX
I_EXTRA += install-probe
endif
endif

all:	$(SRCS) $(TOOLS) $(EXTRA) $(PROBE)

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS) $(EXTRA) probe

cardmgr: cardmgr.o yacc_config.o lex_config.o

yacc_config.o lex_config.o: %.o: %.c
	$(CC) -c -MD -O -pipe $(CPPFLAGS) $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

parser: lex_config.o yacc_config.c
	$(CC) -O $(CPPFLAGS) -DYYDEBUG=1 -o $@ yacc_config.c lex_config.o

cardinfo: cardinfo.o
	$(CC) $< -o $@ $(FLIBS) -lm

install-cardinfo: cardinfo
	@mkdir -p $(PREFIX)/usr/X11R6/bin
	cp -f cardinfo $(PREFIX)/usr/X11R6/bin
	chmod u+s $(PREFIX)/usr/X11R6/bin/cardinfo

install-probe:
	@mkdir -p $(PREFIX)/sbin
	cp -f probe $(PREFIX)/sbin

install: $(TOOLS) $(I_EXTRA)
	@mkdir -p $(PREFIX)/sbin
	@rm -f $(PREFIX)/sbin/cardmgr
	cp -f $(TOOLS) pcinitrd $(PREFIX)/sbin
	chmod u+s $(PREFIX)/sbin/cardctl

include ../rules.mk
