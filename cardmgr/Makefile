#
# cardmgr/Makefile 1.71 1999/10/28 01:22:04 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
XFLAGS = -O -pipe
CPPFLAGS = -I../include -I$(LINUX)/include -I../modules
COFLAGS = -kv
YFLAGS = -d

CC := $(CC) $(UFLAGS)

SRCS = cardmgr.c cardctl.c yacc_config.c lex_config.c probe.c
HDRS = cardmgr.h yacc_config.h
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

all:	$(SRCS) $(HDRS) $(TOOLS) $(EXTRA) $(PROBE)

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f $(TOOLS) $(EXTRA) probe

cardmgr: cardmgr.o yacc_config.o lex_config.o

yacc_config.c yacc_config.h: yacc_config.y
	$(YACC) $(YFLAGS) yacc_config.y
	mv y.tab.c yacc_config.c
	mv y.tab.h yacc_config.h

cardmgr.o: cardmgr.c

yacc_config.o: yacc_config.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -c yacc_config.c

lex_config.o: lex_config.c yacc_config.h
	$(CC) $(XFLAGS) $(CPPFLAGS) -c lex_config.c

parser: lex_config.o yacc_config.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -DYYDEBUG=1 -o parser \
	    yacc_config.c lex_config.o

cardinfo.o: cardinfo.c

cardinfo: cardinfo.o
	$(CC) $(LFLAGS) -o cardinfo cardinfo.o \
	    -L/usr/X11R6/lib -L/usr/X11/lib -lforms -lX11 -lm

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

# Stuff to automatically maintain dependency files

.c.o:
	$(CC) -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(SRCS:%.c=.depfiles/%.d)
