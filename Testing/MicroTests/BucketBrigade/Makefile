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
TARGETS = bucket_brigade_setcontext bucket_brigade_setcontext_jmp bucket_brigade_libuv bucket_brigade_silly_loop

bucket_brigade_setcontext: bucket_brigade_setcontext.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext bucket_brigade_setcontext.c

bucket_brigade_setcontext_jmp: bucket_brigade_setcontext_jmp.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_setcontext_jmp bucket_brigade_setcontext_jmp.c

bucket_brigade_libuv: bucket_brigade_libuv.c
	gcc -g -O1 $(INCLUDE_DIRS) -o bucket_brigade_libuv bucket_brigade_libuv.c $(LIBUV_FLAGS) -pthread

bucket_brigade_pthread: bucket_brigade_pthread.c
	gcc -g -O1 -o bucket_brigade_pthread bucket_brigade_pthread.c -pthread

bucket_brigade_silly_loop: bucket_brigade_silly_loop.c
	gcc -g -O1 -o bucket_brigade_silly_loop bucket_brigade_silly_loop.c

bucket_brigade_crcl: bucket_brigade_crcl.c
	gcc -O1 $(FLAGS) -o bucket_brigade_crcl bucket_brigade_crcl.c -lcharcoal_sys $(LIBUV_FLAGS)

bucket_brigade.class: bucket_brigade.java
	javac bucket_brigade.java

clean:
	rm -rf $(TARGETS)
