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
endif
ifeq ($(OS),Linux)
    LIBUV_FLAGS = ../../Build/libuv/libuv.a
    CCFLAGS += -D LINUX
endif
ifeq ($(OS),Darwin)
    LIBUV_FLAGS = ../../Build/libuv/libuv.a
    LIBUV_FLAGS += -framework CoreFoundation -framework CoreServices
    CCFLAGS += -D OSX
endif


# INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include -I../../ThirdParty/libuv/Releases/0.10.25/include
LIB_DIRS =  -L../../Install/lib
FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg
TARGETS = simple_test simple_test_noyield simple_yield_exe  unyielding_test_exe activity_create timeout_test_exe simple_unyield_test simple_unyield_test_noyield

timer_create_test: timer_create_test.c
	gcc -O1 -o timer_create_test timer_create_test.c -lrt

baton_setcontext: baton_setcontext.c
	gcc -g -O1 $(INCLUDE_DIRS) -o baton_setcontext baton_setcontext.c

baton_setcontext_jmp: baton_setcontext_jmp.c
	gcc -g -O1 $(INCLUDE_DIRS) -o baton_setcontext_jmp baton_setcontext_jmp.c

baton_libuv: baton_libuv.c
	gcc -g -O1 $(INCLUDE_DIRS) -o baton_libuv baton_libuv.c ../../Build/libuv/libuv.a -pthread

smoke_test_01: smoke_test_01.c
	gcc -g -o smoke_test_01 $(FLAGS) -DYIELDING smoke_test_01.c $(LIBUV_FLAGS)


simple_unyield_test:
	gcc -g -o simple_unyield_test $(FLAGS) -DYIELDING simple_unyield_test.c -lm

simple_unyield_test_noyield:
	gcc -g -o simple_unyield_test_noyield $(FLAGS) simple_unyield_test.c -lm 
simple_test:
	gcc -g -o simple_test $(FLAGS) -DYIELDING simple_test.c -lm

simple_test_noyield:
	gcc -g -o simple_test_noyield $(FLAGS) simple_test.c -lm

checks:
	gcc -Wall -o check check_default_stack_size.c -lpthread
	./check

simple_yield:
	gcc -g -o simple_yield_exe $(FLAGS) simple_yield.c
	./simple_yield_exe

activity_create:
	gcc -g -o activity_create $(FLAGS) activity_create.c
	./activity_create

unyielding_test:
	gcc -g -o unyielding_test_exe $(FLAGS) unyielding_test.c

timeout:
	gcc -g -o timeout_test_exe $(FLAGS) timeout_test.c

clean:
	rm -rf $(TARGETS)
