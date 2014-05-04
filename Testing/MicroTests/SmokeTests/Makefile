# TODO: Header
# 
# 

BUILD_DIR=../../Build
CRCL_CPP_EXT=.crcl.cpp.c
CIL_C_EXT=.crcl.cil.c
CC=gcc

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
    LIBUV_FLAGS = ../../../Build/libuv/libuv.a
endif
ifeq ($(OS),Linux)
    CCFLAGS += -D LINUX
endif
ifeq ($(OS),Darwin)
    LIBUV_FLAGS += -framework CoreFoundation -framework CoreServices
    CCFLAGS += -D OSX
endif


# INCLUDE_DIRS = -I../../../Install/include -I../../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
INCLUDE_DIRS = -I../../../Install/include -I../../../ThirdParty/OpenPA/Releases/1.0.4/Install/include -I../../../ThirdParty/libuv/Releases/0.10.25/include
LIB_DIRS =  -L../../../Install/lib
# FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -lrt
TARGETS = hello_world one_activity activate_loop

.SECONDARY:

clean:
	rm -f $(BUILD_DIR)/*

$(BUILD_DIR)/%$(CRCL_CPP_EXT): %.crcl
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	../../../ThirdParty/cil/Charcoal-1.7.3/bin/cilly.native --out $@ $<

$(BUILD_DIR)/hello_world_crcl: $(BUILD_DIR)/hello_world$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/hello_world_crcl $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/one_activity_crcl: $(BUILD_DIR)/one_activity$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/one_activity_crcl $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/hello_world: hello_world.c
	gcc -g -o $(BUILD_DIR)/hello_world $(FLAGS) -DYIELDING hello_world.c $(LIBUV_FLAGS)

$(BUILD_DIR)/hello_world_crcl: $(BUILD_DIR)

one_activity: one_activity.c
	gcc -g -o one_activity $(FLAGS) -DYIELDING one_activity.c $(LIBUV_FLAGS)

activate_loop: activate_loop.c
	gcc -g -o activate_loop $(FLAGS) -DYIELDING activate_loop.c $(LIBUV_FLAGS)
