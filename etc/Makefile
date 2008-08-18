#
# etc/Makefile 1.54 2000/04/07 22:18:39 (David Hinds)
#

# Include site dependent options
include ../config.mk

EXTRA = shared
CLIENTS = config network memory serial ftl scsi ide parport wireless
CLIENT_OPTS = config.opts network.opts memory.opts serial.opts \
	ftl.opts scsi.opts ide.opts parport.opts wireless.opts

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

all:	$(EXTRA) $(CLIENTS) $(CLIENT_OPTS)
ifeq ($(HOST_ARCH),$(ARCH))
	@set -e ; $(MAKE) -C cis
endif

dep:

clean:
ifeq ($(HOST_ARCH),$(ARCH))
	@set -e ; $(MAKE) -C cis clean
endif

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
	    [ -r $$f.conf ] && cp $$f.conf $(ETC)/$$f.conf ;	\
	    cmp -s $$f $(ETC)/$$f && continue ;			\
	    [ -r $(ETC)/$$f ] && mv $(ETC)/$$f $(ETC)/$$f.O ;	\
	    cp $$f $(ETC)/$$f ;					\
	    OPTS=$(ETC)/$$f.opts ;				\
	    test -r $$OPTS || cp $$f.opts $$OPTS ;		\
	done

install-extra: $(EXTRA)
	for f in $(EXTRA) ; do					\
	    cmp -s $$f $(ETC)/$$f && continue ;			\
	    [ -r $(ETC)/$$f ] && mv $(ETC)/$$f $(ETC)/$$f.O ;	\
	    cp $$f $(ETC)/$$f ;					\
	done
	if [ -d /var/state ] ; then				\
	    mkdir -p $(PREFIX)/var/state/pcmcia ;		\
	elif [ -d /var/lib ] ; then				\
	    mkdir -p $(PREFIX)/var/lib/pcmcia ;			\
	fi

install-depmod:
	CONF=$(PREFIX)/etc/modules.conf ;			\
	if [ ! -r $$CONF -a -r $(PREFIX)/etc/conf.modules ] ; then \
	    CONF=$(PREFIX)/etc/conf.modules ;			\
	fi ;							\
	if ! modprobe -c | grep -q 'path\[pcmcia\]' ; then	\
	    grep -qs ^path $$CONF || grep -qs ^keep $$CONF ||	\
		echo keep >> $$CONF ;				\
	    modprobe -c | sed -ne '/path/s/\[net\]/\[pcmcia\]/p' \
		>> $$CONF ;					\
	fi
	if [ "$(PREFIX)" = "" ] ; then depmod -a ; fi

install: $(INST) $(ETC) install-extra install-clients
	set -e ; $(MAKE) -C cis install
