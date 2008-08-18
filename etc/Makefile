#
# etc/Makefile 1.44 1999/05/18 05:29:14 (David Hinds)
#

# Include site dependent options
include ../config.mk

EXTRA = shared
CLIENTS = config network memory serial ftl scsi ide cdrom parport
CLIENT_OPTS = config.opts network.opts memory.opts serial.opts \
	ftl.opts scsi.opts ide.opts cdrom.opts parport.opts

ifdef SYSV_INIT
INST = install-sysv
else
INST = install-bsd
endif

ifdef INSTALL_DEPMOD
INST := $(INST) install-depmod
endif

# Don't change this without editing the device-specific scripts
ETC = $(PREFIX)/etc/pcmcia

# Configuration file for depmod
CONF = $(PREFIX)/etc/conf.modules

all:	$(EXTRA) $(CLIENTS) $(CLIENT_OPTS)
	set -e ; $(MAKE) -C cis

dep:

clean:
	set -e ; $(MAKE) -C cis clean

R=$(PREFIX)$(RC_DIR)
RC_DIRS=$R/init.d $R/rc0.d $R/rc2.d $R/rc3.d $R/rc5.d $R/rc6.d
$(PREFIX)/etc/rc.d $(ETC) $(RC_DIRS):
	mkdir -p $@

PROBE=../cardmgr/probe

BSD=$(PREFIX)/etc/rc.d/rc.pcmcia

install-bsd: $(PREFIX)/etc/rc.d
	if [ -x $(PROBE) ] ; then				\
	    sed -e s/=i82365/=`$(PROBE) -m`/ rc.pcmcia > $(BSD).N ; \
	else cp rc.pcmcia $(BSD).N ; fi
	chmod +x $(BSD).N
	if [ ! -e $(BSD) ] ; then mv $(BSD).N $(BSD) ; fi

install-sysv: $(RC_DIRS)
	if [ -d /etc/sysconfig ] ; then				\
	    mkdir -p $(PREFIX)/etc/sysconfig ;			\
	    CFG=$(PREFIX)/etc/sysconfig/pcmcia ;		\
	    if [ -f $$CFG ] ; then . $$CFG ; fi;		\
	    if [ "$$PCMCIA" != "yes" ] ; then			\
		echo PCMCIA=yes > $$CFG ;			\
		if [ -x $(PROBE) ] ; then			\
		    echo PCIC=`../cardmgr/probe -m` >> $$CFG ;	\
		else						\
		    echo PCIC=i82365 >> $$CFG ;			\
		fi ; 						\
		echo PCIC_OPTS= >> $$CFG ;			\
		echo CORE_OPTS= >> $$CFG ;			\
		echo CARDMGR_OPTS= >> $$CFG ;			\
	    fi ;						\
	fi
	cp rc.pcmcia $(PREFIX)$(RC_DIR)/init.d/pcmcia
	chmod +x $(PREFIX)$(RC_DIR)/init.d/pcmcia
	for RC in $(PREFIX)$(RC_DIR)/rc[06].d ; do		\
	    if [ ! -r $$RC/*pcmcia ] ; then			\
		ln -sf ../init.d/pcmcia $$RC/K52pcmcia ;	\
	    fi ;						\
	done
	for RC in $(PREFIX)$(RC_DIR)/rc[235].d ; do		\
	    if [ ! -r $$RC/*pcmcia ] ; then			\
		ln -sf ../init.d/pcmcia $$RC/S45pcmcia ;	\
	    fi ;						\
	done

install-clients:
	for f in $(CLIENTS) ; do				\
	    cmp -s $$f $(ETC)/$$f || cp -b -Vt $$f $(ETC)/$$f ; \
	    DEST=$(ETC)/$$f.opts ;				\
	    test -r $$DEST || cp $$f.opts $$DEST ;		\
	done

install-extra: $(EXTRA)
	for f in $(EXTRA) ; do					\
	    cmp -s $$f $(ETC)/$$f || cp -b -Vt $$f $(ETC)/$$f ; \
	done

install-depmod:
	grep -qs path $(CONF) || echo keep >> $(CONF)
	grep -q pcmcia $(CONF) || \
	    echo 'path[pcmcia]=/lib/modules/`uname -r`' >> $(CONF) && \
	    echo 'path[pcmcia]=/lib/modules/default' >> $(CONF) && \
	    echo 'path[pcmcia]=/lib/modules/preferred' >> $(CONF)
	if [ "$(PREFIX)" = "" ] ; then depmod -a ; fi

install: $(INST) $(ETC) install-extra install-clients
	set -e ; $(MAKE) -C cis install
