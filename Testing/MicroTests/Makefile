

checks:
	gcc -Wall -o check check_default_stack_size.c
	./check

all:
	gcc -g -o activity_create -I../../Install/include -I../../ThirdParty/OpenPA/Releases/1.0.4/Install/include -L../../Install/lib -lcharcoal_sys activity_create.c
	./activity_create
