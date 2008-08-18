#
# Makefile 1.30 1997/10/05 22:54:47 (David Hinds)
#

DIRS = modules cardmgr flash debug-tools man etc

help:
	@echo "Pick one of the following targets:"
	@echo -e "\tmake config\t\t- configure and check system setup"
	@echo -e "\tmake all\t\t- build modules and programs"
	@echo -e "\tmake install\t\t- install modules and programs"

config .prereq.ok:
	@./Configure

all:	.prereq.ok kcheck
	set -e ; for d in $(DIRS) ; do $(MAKE) -C $$d ; done
	for f in *.mk ; do if [ $$f != config.mk ] ; then \
	    $(MAKE) -f $$f all ; \
	fi ; done

clean:
	set -e ; for d in $(DIRS) ; do $(MAKE) -C $$d clean ; done
	rm -f .prereq.ok config.out config.mk modules/config.h
	rm -f include/linux/modversions.h

install: .prereq.ok kcheck
	set -e ; for d in $(DIRS) ; do $(MAKE) -C $$d install ; done
	for f in *.mk ; do if [ $$f != config.mk ] ; then \
	    $(MAKE) -f $$f install ; \
	fi ; done

kcheck:
	@. config.out ; \
	if [ "$$CHECK" != "" ] ; then \
	    if [ "`cksum < $$CHECK`" != "$$CKSUM" ] ; then \
		/bin/echo -n "Kernel configuration has changed." ; \
		/bin/echo "  Please re-run 'make config'." ; \
		exit 1 ; \
	    fi ; \
	fi
