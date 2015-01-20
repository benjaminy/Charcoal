# TODO: Header
# 
# 

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
FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg -lrt
TARGETS = spawn_pthread

spawn_pthread: spawn_pthread.c
	gcc -g -O1 -o spawn_pthread spawn_pthread.c -pthread

clean:
	rm -rf $(TARGETS)
