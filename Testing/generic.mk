
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
    LIBUV_FLAGS = $(CRCL_ROOT)/Build/libuv/libuv.a
endif
ifeq ($(OS),Linux)
    CCFLAGS += -D LINUX
endif
ifeq ($(OS),Darwin)
    LIBUV_FLAGS += -framework CoreFoundation -framework CoreServices
    CCFLAGS += -D OSX
endif

BUILD_DIR=$(CRCL_ROOT)/Testing/Build
CIL_DIR=$(CRCL_ROOT)/ThirdParty/cil/Charcoal-1.7.3
CRCL_CPP_EXT=.crcl.cpp.c
CIL_C_EXT=.crcl.cil.c
CC=gcc

# INCLUDE_DIRS = -I$(CRCL_ROOT)/Install/include -I$(CRCL_ROOT)/ThirdParty/OpenPA/Releases/1.0.4/Install/include
INCLUDE_DIRS = -I$(CRCL_ROOT)/Install/include -I$(CRCL_ROOT)/ThirdParty/OpenPA/Releases/1.0.4/Install/include -I$(CRCL_ROOT)/ThirdParty/libuv/Releases/0.10.25/include
LIB_DIRS =  -L$(CRCL_ROOT)/Install/lib
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
FLAGS = -g $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -lrt


# # INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
# INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include -I../../ThirdParty/libuv/Releases/0.10.25/include
# LIB_DIRS =  -L../../Install/lib
# # FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -lrt

# # INCLUDE_DIRS = -I../../../Install/include -I../../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
# INCLUDE_DIRS = -I../../../Install/include -I../../../ThirdParty/OpenPA/Releases/1.0.4/Install/include -I../../../ThirdParty/libuv/Releases/0.10.25/include
# LIB_DIRS =  -L../../../Install/lib
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
