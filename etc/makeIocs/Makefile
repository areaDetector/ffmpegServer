TOP = ../..
include $(TOP)/configure/CONFIG

# include the list of IOCs
include $(TOP)/etc/makeIocs/Makefile.iocs

# this is the dls-xml-iocbuilder.py file
XMLBUILDER := dls-xml-iocbuilder.py
#XMLBUILDER := /dls_sw/work/common/python/iocbuilder/xmlbuilder/xmlbuilder.py

# set to -d to get debugging
DEBUG :=

# Add the created iocs and sims to the install target
TARGETS += $(IOC_DIRS)

# include standard rules
include $(TOP)/configure/RULES

# General rule for building a Standard IOC from an XML file
$(TOP)/iocs/%: ../%.xml $(wildcard ../%_RELEASE)
	$(RMDIR) ../O.Common/tmp$(*F) $@     
	$(XMLBUILDER) $(DEBUG) -e -o ../O.Common/tmp$(*F) $<
	$(MV) ../O.Common/tmp$(*F)/$(*F) $@

# General rule for building a Simulation IOC from an XML file
$(TOP)/iocs/%_sim: ../%.xml $(wildcard ../%_RELEASE)
	$(RMDIR) ../O.Common/tmp$(*F) $@
	$(XMLBUILDER) $(DEBUG) --sim=linux-x86 -e -o ../O.Common/tmp$(*F) $<
	$(MV) ../O.Common/tmp$(*F)/$(*F)_sim $@

# Alternate rule for building a Standard IOC from a python file
$(TOP)/iocs/%: ../%.py
	$(RMDIR) ../O.Common/tmp$(*F) $@
	./$< $(DEBUG) -e -o ../O.Common/tmp$(*F) $*
	$(MV) ../O.Common/tmp$(*F)/$(*F) $@

# Alternate rule for building a Simulation IOC from a python file
$(TOP)/iocs/%_sim: ../%.py
	$(RMDIR) ../O.Common/tmp$(*F) $@
	./$< $(DEBUG) --sim=linux-x86 -e -o ../O.Common/tmp$(*F) $*
	$(MV) ../O.Common/tmp$(*F)/$(*F)_sim $@

# Special rule to get rid of installed ioc
uninstall:
	$(RMDIR) $(IOC_DIRS)
