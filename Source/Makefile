# _very_ incomplete

BUILD_DIR = ../Build
INSTALL_DIR = ../Install
LIB_DIR = $(INSTALL_DIR)/lib
INCLUDE_DIR = $(INSTALL_DIR)/include
INCLUDE_DIRS = -I. -I$(INCLUDE_DIR)
CFLAGS += -Wall -O1 -g

RUNTIME_PIECES = io_commands coroutine_threads coroutine_main coroutine
MORE_PIECES = $(addprefix runtime_, $(RUNTIME_PIECES)) semaphore
RUNTIME_OBJS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/charcoal_, $(MORE_PIECES)))

RUNTIME_HEADER_PIECES = common io_commands atomics coroutine
MORE_HEADER_PIECES = $(addprefix runtime_, $(RUNTIME_HEADER_PIECES)) semaphore non_c
HEADERS = $(addsuffix .h, $(addprefix charcoal_, $(MORE_HEADER_PIECES)) charcoal application_pre_header)

all: install_headers $(RUNTIME_OBJS) std_lib
	mkdir -p $(LIB_DIR)
	ar rcs $(LIB_DIR)/libcharcoal_sys.a $(RUNTIME_OBJS) $(BUILD_DIR)/charcoal_std_lib.o
	mkdir -p $(INCLUDE_DIR)

clean:
	rm -rf $(BUILD_DIR)/*
	rm -f $(addprefix $(INCLUDE_DIR)/, $(HEADERS))
	rm -f $(LIB_DIR)/libcharcoal_sys.a

install_headers: $(addprefix $(INCLUDE_DIR)/, $(HEADERS))

$(INCLUDE_DIR)/%.h : %.h
	cp $< $@

$(BUILD_DIR)/%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

std_lib : $(BUILD_DIR)/charcoal_std_lib.o

runtime : $(BUILD_DIR)/charcoal_runtime.o
