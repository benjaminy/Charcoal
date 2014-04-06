# _very_ incomplete

# INCLUDE_DIRS = -I. -I../ThirdParty/OpenPA/Releases/1.0.4/Install/include
# INCLUDE_DIRS = -I. -I /usr/local/stow/openpa-1.0.4/include
INCLUDE_DIRS = -I. -I../ThirdParty/libuv/Releases/0.10.25/include

CFLAGS += -g -pg -Wall

all: semaphore runtime runtime_io_commands
	gcc $(CFLAGS) -c -o ../Build/charcoal_runtime_io_commands.o $(INCLUDE_DIRS) charcoal_runtime_io_commands.c
	gcc $(CFLAGS) -c -o ../Build/charcoal_std_lib.o charcoal_std_lib.c
	mkdir -p ../Install/lib
	ar rcs ../Install/lib/libcharcoal_sys.a ../Build/libuv/libuv.a ../Build/charcoal_runtime.o ../Build/charcoal_runtime_io_commands.o ../Build/charcoal_semaphore.o ../Build/charcoal_std_lib.o
	mkdir -p ../Install/include
	cp charcoal_base.h ../Install/include
	cp charcoal_semaphore.h ../Install/include
	cp charcoal_runtime.h ../Install/include
	cp charcoal_runtime_atomics.h ../Install/include
	cp charcoal_std_lib.h ../Install/include

../Build/charcoal_semaphore.o : charcoal_semaphore.c charcoal_semaphore.h charcoal_base.h
	gcc $(CFLAGS) -c -o ../Build/charcoal_semaphore.o $(INCLUDE_DIRS) charcoal_semaphore.c

semaphore : ../Build/charcoal_semaphore.o

../Build/charcoal_runtime.o : charcoal_runtime.c charcoal_runtime.h charcoal_base.h
	gcc $(CFLAGS) -c -o ../Build/charcoal_runtime.o $(INCLUDE_DIRS) charcoal_runtime.c

runtime_io_commands : ../Build/charcoal_runtime_io_commands.o

../Build/charcoal_runtime_io_commands.o : charcoal_runtime_io_commands.c charcoal_runtime_io_commands.h charcoal_base.h
	gcc $(CFLAGS) -c -o ../Build/charcoal_runtime_io_commands.o $(INCLUDE_DIRS) charcoal_runtime_io_commands.c

runtime : ../Build/charcoal_runtime.o
