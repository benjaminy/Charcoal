# Foo

# PIN_ROOT must be defined and this Makefile must be invoked with the
# -e option to prevent it from being overridden by the stupid Pin
# makefiles.

PINTOOL    ?= You_must_define_PINTOOL
GUEST_CMD  ?= /bin/ls
# TOP            ?= $(shell pwd)
# PIN_VERSION_NUM = 2.13
# PIN_REPO_NUM    = 61206
# PIN_COMPILER    = gcc.4.4.7
# # PIN_COMPILER  = clang.4.2
# PIN_OS  	= linux
# # PIN_OS	= mac
# PIN_REL_DIR     = pin-$(PIN_VERSION_NUM)-$(PIN_REPO_NUM)-$(PIN_COMPILER)-$(PIN_OS)
# PIN_ROOT        = $(HOME)/Research/$(PIN_REL_DIR)
# PIN_KIT         = $(PIN_ROOT)
# PIN_HOME        = $(PIN_ROOT)/source/tools
# PIN_TOOLS       = $(PIN_HOME)


all:
	@echo There is no default action in this Makefile

CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
include $(PIN_ROOT)/source/tools/SimpleExamples/makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules

OBJDIR          = obj-intel64/
DLL_SUFFIX      = .so
# DLL_SUFFIX      = .dylib

# TOOL_CXXFLAGS += -I.
# TOOL_CXXFLAGS += --std=c++11
# TOOL_CXXFLAGS += -lc

TOOL_CXXFLAGS += -g

# compile a pintool
cpintool: $(OBJDIR)$(PINTOOL)$(DLL_SUFFIX)

# run a pintool
rpintool: $(OBJDIR)$(PINTOOL)$(DLL_SUFFIX)
	$(PIN_ROOT)/pin -follow_execv 1 -injection dynamic -t $(OBJDIR)$(PINTOOL)$(DLL_SUFFIX) -- $(GUEST_CMD)

Testing/thread_sleep: Testing/thread_sleep.c
	gcc -Wall -g -o Testing/thread_sleep Testing/thread_sleep.c -pthread

Testing/thread_sleep2: Testing/thread_sleep2.c
	gcc -Wall -g -o Testing/thread_sleep2 Testing/thread_sleep2.c -pthread

Testing/thread_sleep3: Testing/thread_sleep3.c
	gcc -Wall -g -o Testing/thread_sleep3 Testing/thread_sleep3.c -pthread

Testing/simple_data_race_1: Testing/simple_data_race_1.c
	gcc -Wall -g -o Testing/simple_data_race_1 Testing/simple_data_race_1.c -pthread

Testing/simple_data_race_2: Testing/simple_data_race_2.c
	gcc -Wall -g -o Testing/simple_data_race_2 Testing/simple_data_race_2.c -pthread

Testing/simple_data_race_3: Testing/simple_data_race_3.c
	gcc -Wall -g -o Testing/simple_data_race_3 Testing/simple_data_race_3.c -pthread

Testing/simple_data_race_4: Testing/simple_data_race_4.c
	gcc -Wall -g -o Testing/simple_data_race_4 Testing/simple_data_race_4.c -pthread

testfoo: $(OBJDIR)thread_fuss$(DLL_SUFFIX) Testing/thread_sleep Testing/thread_sleep2 Testing/thread_sleep3
	$(PIN_ROOT)/pin -follow_execv 1 -injection dynamic -t $(OBJDIR)thread_fuss$(DLL_SUFFIX) -- Testing/thread_sleep3

testrace: $(OBJDIR)thread_fuss$(DLL_SUFFIX) Testing/simple_data_race_1
	$(PIN_ROOT)/pin -follow_execv 1 -injection dynamic -t $(OBJDIR)thread_fuss$(DLL_SUFFIX) -- Testing/simple_data_race_1

testrace2: $(OBJDIR)thread_fuss$(DLL_SUFFIX) Testing/simple_data_race_2
	$(PIN_ROOT)/pin -follow_execv 1 -injection dynamic -t $(OBJDIR)thread_fuss$(DLL_SUFFIX) -- Testing/simple_data_race_2

clean:
	-rm -rf $(OBJDIR) *.out *.log *.tested *.failed *.d *makefile.copy *.exp *.lib

$(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX): bbl_queue.cpp
	cp bbl_queue.cpp $(PIN_TOOLS)/SimpleExamples/
	make -C $(PIN_TOOLS)/SimpleExamples $(OBJDIR)bbl_queue$(DLL_SUFFIX)

bbl_queue: $(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX)
	make $(PIN_TOOLS)/SimpleExamples/$(OBJDIR)bbl_queue$(DLL_SUFFIX)


runstuff:
	~/Research/QueueExperimentCode/simple_run_pin.sh -o_base /tmp/zzzz07/bbl_cfg -- runspec --config=quor-gcc-v1 --size=test --noreportable --tune=base --iterations=1 gcc

# OBJDIR=obj-intel64/
# DLL_SUFFIX=.dylib

# echo $PIN_ROOT/pin -follow_execv 1 -injection dynamic -t $PIN_TOOLS/SimpleExamples/${OBJDIR}bbl_queue${DLL_SUFFIX} $*
# time $PIN_ROOT/pin -follow_execv 1 -injection dynamic -t $PIN_TOOLS/SimpleExamples/${OBJDIR}bbl_queue${DLL_SUFFIX} $*
