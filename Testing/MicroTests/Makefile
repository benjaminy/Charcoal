
# INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
INCLUDE_DIRS = -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include
LIB_DIRS =  -L../../Install/lib
FLAGS = $(INCLUDE_DIRS) $(LIB_DIRS) -lcharcoal_sys -lpthread -pg

checks:
	gcc -Wall -o check check_default_stack_size.c -lpthread
	gcc -g -o dumb $(FLAGS) dumb.c
	gcc -g -o dumber $(FLAGS) dumber.c
	./check

simple_yield:
	gcc -g -o simple_yield_exe $(FLAGS) simple_yield.c
	./simple_yield_exe
#all is a lie
all:
	gcc -g -o activity_create $(FLAGS) activity_create.c
	./activity_create

unyielding_test:
	gcc -g -o unyielding_test_exe $(FLAGS) unyielding_test.c

timeout:
	gcc -g -o timeout_test_exe $(FLAGS) timeout_test.c
