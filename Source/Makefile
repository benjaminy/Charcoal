# _very_ incomplete

BUILD_DIR = ../Build
INSTALL_DIR = ../Install
INCLUDE_DIRS = -I. -I$(INSTALL_DIR)/include
HEADER_INSTALL_DIR = $(INSTALL_DIR)/include
CFLAGS += -Wall -O1 -g

RUNTIME_PIECES = io_commands coroutine_threads coroutine_main coroutine
MORE_PIECES = $(addprefix runtime_, $(RUNTIME_PIECES)) semaphore
RUNTIME_OBJS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/charcoal_, $(MORE_PIECES)))

RUNTIME_HEADER_PIECES = common io_commands atomics coroutine
MORE_HEADER_PIECES = $(addprefix runtime_, $(RUNTIME_HEADER_PIECES)) semaphore non_c
HEADERS = $(addsuffix .h, $(addprefix charcoal_, $(MORE_HEADER_PIECES)) charcoal)

all: install_headers $(RUNTIME_OBJS) std_lib
	mkdir -p ../Install/lib
	ar rcs ../Install/lib/libcharcoal_sys.a $(RUNTIME_OBJS) ../Build/charcoal_std_lib.o
	mkdir -p ../Install/include

clean:
	rm -f ../Build/*.o
	rm -f ../Install/lib/libcharcoal_sys.a
	rm -fr ../Install/include/*

install_headers: $(addprefix $(HEADER_INSTALL_DIR)/, $(HEADERS))

$(HEADER_INSTALL_DIR)/%.h : %.h
	cp $< $@

$(BUILD_DIR)/%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

../Build/charcoal_std_lib.o : charcoal_std_lib.c charcoal_std_lib.h charcoal.h
	gcc $(CFLAGS) -c -o ../Build/charcoal_std_lib.o $(INCLUDE_DIRS) charcoal_std_lib.c

std_lib : ../Build/charcoal_std_lib.o

../Build/charcoal_runtime.o : charcoal_runtime.c charcoal_runtime_common.h charcoal_runtime_coroutine.h charcoal.h
	gcc $(CFLAGS) -c -o ../Build/charcoal_runtime.o $(INCLUDE_DIRS) charcoal_runtime.c

runtime : ../Build/charcoal_runtime.o
