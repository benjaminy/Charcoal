
# INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
LIB_DIRS =  -L../../Install/lib
FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys

checks:
	gcc -Wall -o check check_default_stack_size.c -lpthread
	./check

simple_yield:
	gcc -g -o simple_yield_exe $(FLAGS) simple_yield.c
	./simple_yield_exe

all:
	gcc -g -o activity_create $(FLAGS) activity_create.c
	./activity_create
