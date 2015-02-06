
BUILD_DIR=$(CRCL_ROOT)/Testing/Build
# XXX replace cil dir with install
CIL_DIR=$(CRCL_ROOT)/ThirdParty/cil/Charcoal-1.7.3
INSTALL_DIR=$(CRCL_ROOT)/Install

ifeq ($(OS),Windows_NT)
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS = Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        OS = Darwin
    endif
endif

ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32
else
    LIBUV_FLAGS = $(INSTALL_DIR)/lib/libuv.a
endif
ifeq ($(OS),Linux)
    CCFLAGS += -D LINUX
endif
ifeq ($(OS),Darwin)
    LIBUV_FLAGS += -framework CoreFoundation -framework CoreServices
    CCFLAGS += -D OSX
endif

CRCL_CPP_EXT=.crcl.cpp.c
CIL_C_EXT=.crcl.cil.c
CC=gcc

INCLUDE_DIRS = -I$(INSTALL_DIR)/include
LIB_DIRS =  -L$(INSTALL_DIR)/lib
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
# FLAGS = -g $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -lrt
FLAGS = -g $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys
