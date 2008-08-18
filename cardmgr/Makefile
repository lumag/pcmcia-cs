#
# cardmgr/Makefile 1.62 1998/04/27 08:10:23 (David Hinds)
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

SRCS = cardmgr.c cardctl.c yacc_config.c lex_config.c probe.c
HDRS = cardmgr.h
TOOLS = cardmgr cardctl

ifdef CONFIG_INET
SRCS := ${SRCS} ifport.c ifuser.c
TOOLS := ${TOOLS} ifport ifuser
endif

ifdef CONFIG_SCSI
SRCS := ${SRCS} scsi_info.c
TOOLS := ${TOOLS} scsi_info
endif

ifdef CONFIG_BLK_DEV_IDE_PCMCIA
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

all:	$(SRCS) $(HDRS) $(TOOLS) $(EXTRA) $(PROBE)

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS) $(EXTRA) probe

cardmgr: cardmgr.o yacc_config.o lex_config.o

cardmgr.o: cardmgr.c

yacc_config.o: yacc_config.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c yacc_config.c

lex_config.o: lex_config.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c lex_config.c

parser: lex_config.o yacc_config.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -DDEBUG -o parser yacc_config.c lex_config.o

cardinfo: cardinfo.c
	$(CC) $(LFLAGS) $(CFLAGS) $(CPPFLAGS) -o cardinfo cardinfo.c \
	    -L/usr/X11R6/lib -L/usr/X11/lib \
	    -L/usr/local/lib -lforms -lX11 -lm

install: $(TOOLS) $(EXTRA) $(PROBE)
	mkdir -p $(PREFIX)/sbin
	cp -f $(TOOLS) pcinitrd $(PREFIX)/sbin
	chmod u+s $(PREFIX)/sbin/cardctl
	if [ "$(PREFIX)" != "" -a -f probe ] ; then	\
	    cp -f probe $(PREFIX)/sbin ;		\
	fi
	if [ -r cardinfo ] ; then			\
	    mkdir -p $(PREFIX)/usr/bin/X11 ;		\
	    cp -f cardinfo $(PREFIX)/usr/bin/X11 ;	\
	    chmod u+s $(PREFIX)/usr/bin/X11/cardinfo ;	\
	fi

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
