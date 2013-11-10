# Foo

GUEST_CMD      ?= /bin/ls
TOP            ?= $(shell pwd)
PIN_VERSION_NUM = 2.13
PIN_REPO_NUM    = 61206
PIN_COMPILER    = gcc.4.4.7
# PIN_COMPILER  = clang.4.2
PIN_OS  	= linux
# PIN_OS	= mac
PIN_REL_DIR     = pin-$(PIN_VERSION_NUM)-$(PIN_REPO_NUM)-$(PIN_COMPILER)-$(PIN_OS)
PIN_ROOT        = $(HOME)/Research/$(PIN_REL_DIR)
PIN_KIT         = $(PIN_ROOT)
PIN_HOME        = $(PIN_ROOT)/source/tools
PIN_TOOLS       = $(PIN_HOME)


CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include $(PIN_ROOT)/source/tools/SimpleExamples/makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules

OBJDIR          = obj-intel64/
DLL_SUFFIX      = .so
# DLL_SUFFIX      = .dylib

all:
	@echo $(FOO)

clean:
	-rm -rf $(OBJDIR) *.out *.log *.tested *.failed *.d *makefile.copy *.exp *.lib

$(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX): bbl_queue.cpp
	cp bbl_queue.cpp $(PIN_TOOLS)/SimpleExamples/
	make -C $(PIN_TOOLS)/SimpleExamples $(OBJDIR)bbl_queue$(DLL_SUFFIX)

bbl_queue: $(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX)
	make $(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX)

hme:
	ghc --make -Wall miscellaneous_utilities.hs graph_utilities.hs interface_utilities.hs queue_monster.hs

cme:
	gcc -Wall -g -o quoroboros_workhorse -I/opt/local/include -lpcre quoroboros_workhorse.c pointer_set.c parse_cfg_file.c

runstuff:
	~/Research/QueueExperimentCode/simple_run_pin.sh -o_base /tmp/zzzz07/bbl_cfg -- runspec --config=quor-gcc-v1 --size=test --noreportable --tune=base --iterations=1 gcc

# OBJDIR=obj-intel64/
# DLL_SUFFIX=.dylib

# echo $PIN_ROOT/pin -follow_execv 1 -injection dynamic -t $PIN_TOOLS/SimpleExamples/${OBJDIR}bbl_queue${DLL_SUFFIX} $*
# time $PIN_ROOT/pin -follow_execv 1 -injection dynamic -t $PIN_TOOLS/SimpleExamples/${OBJDIR}bbl_queue${DLL_SUFFIX} $*
