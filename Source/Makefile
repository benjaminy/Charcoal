# _very_ incomplete

# INCLUDE_DIRS = -I. -I../ThirdParty/OpenPA/Releases/1.0.4/Install/include
# INCLUDE_DIRS = -I. -I /usr/local/stow/openpa-1.0.4/include -pg
INCLUDE_DIRS = -I. -I../ThirdParty/libuv/Releases/0.10.25/include -pg

all:
	gcc -g -Wall -c -o ../Build/charcoal_runtime.o $(INCLUDE_DIRS) charcoal_runtime.c
	gcc -g -Wall -c -o ../Build/charcoal_runtime_io_commands.o $(INCLUDE_DIRS) charcoal_runtime_io_commands.c
	gcc -g -Wall -c -o ../Build/charcoal_std_lib.o charcoal_std_lib.c
	mkdir -p ../Install/lib
	ar rcs ../Install/lib/libcharcoal_sys.a ../Build/libuv/libuv.a ../Build/charcoal_runtime.o ../Build/charcoal_std_lib.o
	mkdir -p ../Install/include
	cp charcoal_runtime.h ../Install/include
	cp charcoal_std_lib.h ../Install/include
