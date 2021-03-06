#
#
#

ifndef CRCL_ROOT
$(error CRCL_ROOT is not set)
endif

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
    LIBUV_FLAGS = $(INSTALL_DIR)/lib/libuv.a -pthread
endif
ifeq ($(OS),Linux)
    CCFLAGS += -D LINUX
endif
ifeq ($(OS),Darwin)
    LIBUV_FLAGS += -framework CoreFoundation -framework CoreServices
    CCFLAGS += -D OSX
endif

CRCL_DOT_H_EXT=.crcl.with.h_c
CRCL_CPP_EXT=.crcl.cpp_c
CIL_C_EXT=.crcl.cil_c
CRCL_CJ_EXT=.crcl.cilj_c
CRCL_O_EXT=.crcl.cil_o
CC=clang-3.6

CRCL_RUNTIME = $(INSTALL_DIR)/lib/libcharcoal_sys.a
ZLOG_LIB = $(INSTALL_DIR)/lib/libzlog.a

INCLUDE_DIRS = -I$(INSTALL_DIR)/include
LIB_DIRS =  -L$(INSTALL_DIR)/lib
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
# FLAGS = -g $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -lrt
# CFLAGS = -g
