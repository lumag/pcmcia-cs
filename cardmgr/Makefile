#
# cardmgr/Makefile 1.79 2000/01/21 23:26:12 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
CPPFLAGS := $(CPPFLAGS) -I../modules

CC := $(CC) $(UFLAGS)

SRCS = cardmgr.c cardctl.c yacc_config.c lex_config.c probe.c
TOOLS = cardmgr cardctl

ifdef CONFIG_INET
SRCS := ${SRCS} ifport.c ifuser.c
TOOLS := ${TOOLS} ifport ifuser
endif

ifdef CONFIG_SCSI
SRCS := ${SRCS} scsi_info.c
TOOLS := ${TOOLS} scsi_info
endif

ifdef DO_IDE
SRCS := ${SRCS} ide_info.c
TOOLS := ${TOOLS} ide_info
endif

ifdef HAS_FORMS
SRCS := ${SRCS} cardinfo.c
CPPFLAGS := ${CPPFLAGS} -I/usr/X11R6/include -I/usr/X11/include \
	-I/usr/X11R6/include/X11
EXTRA := ${EXTRA} cardinfo
endif

ifdef CONFIG_ISA
PROBE = probe
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
	$(CC) $< -o $@ -L/usr/X11R6/lib -L/usr/X11/lib -lforms -lX11 -lm

install: $(TOOLS) $(EXTRA) $(PROBE)
	mkdir -p $(PREFIX)/sbin
	rm -f $(PREFIX)/sbin/cardmgr
	cp -f $(TOOLS) pcinitrd $(PREFIX)/sbin
	chmod u+s $(PREFIX)/sbin/cardctl
	if [ "$(PREFIX)" != "" -a -f probe ] ; then	\
	    cp -f probe $(PREFIX)/sbin ;		\
	fi
	if [ -r cardinfo ] ; then			\
	    mkdir -p $(PREFIX)/usr/X11R6/bin ;		\
	    cp -f cardinfo $(PREFIX)/usr/X11R6/bin ;	\
	    chmod u+s $(PREFIX)/usr/X11R6/bin/cardinfo ;\
	fi

include ../rules.mk
