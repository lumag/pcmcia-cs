#
# cardmgr/Makefile 1.92 2002/12/21 08:22:25 (David Hinds)
#

# Include site dependent options
include ../config.mk

CPPFLAGS += -I../modules
CC = $(UCC) $(UFLAGS)

SRCS  = cardmgr.c cardctl.c yacc_config.c lex_config.c pcic_probe.c
TOOLS = cardmgr cardctl

ifdef CONFIG_INET
SRCS	+= ifport.c ifuser.c
TOOLS	+= ifport ifuser
endif

ifdef CONFIG_SCSI
SRCS	+= scsi_info.c
TOOLS	+= scsi_info
endif

ifdef DO_IDE
SRCS	+= ide_info.c
TOOLS	+= ide_info
endif

XCPPFLAGS = -I/usr/X11R6/include -I/usr/X11/include -I/usr/X11R6/include/X11

ifdef HAS_FORMS
SRCS 	+= cardinfo.c
EXTRA	+= cardinfo
I_EXTRA	+= install-cardinfo
endif

ifdef HAS_XAW
SRCS 	+= xcardinfo.c
EXTRA	+= xcardinfo
I_EXTRA	+= install-xcardinfo
endif

ifdef HAS_GTK
SRCS	+= gpccard.c
EXTRA	+= gpccard
I_EXTRA	+= install-gpccard
endif

ifdef CONFIG_ISA
PROBE = pcic_probe
ifdef PREFIX
I_EXTRA += install-probe
endif
endif

all:	$(SRCS) $(TOOLS) $(EXTRA) $(PROBE)

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS) $(EXTRA) pcic_probe

cardmgr: cardmgr.o yacc_config.o lex_config.o

yacc_config.o lex_config.o: %.o: %.c
	$(CC) -c -MD -O -pipe $(CPPFLAGS) $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

parser: lex_config.o yacc_config.c
	$(CC) -O $(CPPFLAGS) -DYYDEBUG=1 -o $@ yacc_config.c lex_config.o

cardinfo.o: cardinfo.c
	$(CC) $(CFLAGS) -MD $(CPPFLAGS) $(XCPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

cardinfo: cardinfo.o
	$(CC) $< -o $@ $(FLIBS)

xcardinfo.o: xcardinfo.c
	$(CC) $(CFLAGS) -MD $(CPPFLAGS) $(XCPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

xcardinfo: xcardinfo.o
	$(CC) $< -o $@ -L/usr/X11R6/lib -L/usr/X11/lib -lX11 -lXaw

gpccard.o: gpccard.c
	$(CC) -O -Wall -MD $(CPPFLAGS) `gtk-config --cflags` -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

gpccard: gpccard.o
	$(CC) $< -o $@ `gtk-config --libs`

install-cardinfo: cardinfo
	@mkdir -p $(PREFIX)/usr/X11R6/bin
	cp -f cardinfo $(PREFIX)/usr/X11R6/bin
	chmod u+s $(PREFIX)/usr/X11R6/bin/cardinfo

install-xcardinfo: xcardinfo
	@mkdir -p $(PREFIX)/usr/X11R6/bin
	cp -f xcardinfo $(PREFIX)/usr/X11R6/bin
	chmod u+s $(PREFIX)/usr/X11R6/bin/xcardinfo

install-gpccard: gpccard
	@mkdir -p $(PREFIX)/usr/bin
	cp -f gpccard $(PREFIX)/usr/bin

install-probe: pcic_probe
	@mkdir -p $(PREFIX)/sbin
	cp -f pcic_probe $(PREFIX)/sbin

install: $(TOOLS) $(I_EXTRA)
	@mkdir -p $(PREFIX)/sbin
	@rm -f $(PREFIX)/sbin/cardmgr
	cp -f $(TOOLS) pcinitrd $(PREFIX)/sbin
	chmod u+s $(PREFIX)/sbin/cardctl

include ../rules.mk
