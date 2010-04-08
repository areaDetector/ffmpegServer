TOP = ../..
include $(TOP)/configure/CONFIG

# set to -d to get debugging
DEBUG :=

# this is the doxygen output dir
DOCDIR := $(TOP)/documentation/doxygen

# add the documentation files to the install target
install: $(DOCDIR)

# rule for documentation
$(DOCDIR): config.src manual.src $(DOCDIR)/build_instructions
	mkdir -p $(DOCDIR)
	dls-make-doxygen-documentation.py -o $(DOCDIR) config.src manual.src

# rule for generating build instructions from example.xml file
$(DOCDIR)/build_instructions: $(TOP)/etc/makeIocs/example.xml
	mkdir -p $(DOCDIR)
	dls-xml-iocbuilder.py --doc=$@ $(DEBUG) $^

# Remove entire documentation/doxygen dir on clean
clean:
	rm -rf $(DOCDIR)
