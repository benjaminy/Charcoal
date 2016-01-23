# _very_ incomplete

OPTIMIZATION_LEVEL = -O1
DEBUG_SYMBOLS = -g
# PROFILING = ""
# SANITIZE = "-fsanitize=address"
SANITIZE = -fsanitize=address

CC = clang-3.6
BUILD_DIR = ../Build
include charcoal_pre.mk

STDLIB_DIR = StandardLibrary
LIB_DIR = $(INSTALL_DIR)/lib
ETC_DIR = $(INSTALL_DIR)/etc
INCLUDE_DIR = $(INSTALL_DIR)/include
INCLUDE_DIRS = -I. -I $(INCLUDE_DIR)
CFLAGS += -Wall $(OPTIMIZATION_LEVEL) $(DEBUG_SYMBOLS) $(PROFILING) $(SANITIZE)

# charcoal_log.conf

STDLIB_SOURCES = unistd lib_utils semaphore
STDLIB_OBJS = $(addsuffix $(CRCL_O_EXT), $(addprefix $(BUILD_DIR)/$(STDLIB_DIR)/, $(STDLIB_SOURCES) ) )
RUNTIME_SOURCES = coroutine main threads io_commands semaphore
RUNTIME_C_OBJS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/runtime_, $(RUNTIME_SOURCES)) $(BUILD_DIR)/setjmp)
# RUNTIME_CRCL_OBJS = $(addsuffix $(CRCL_O_EXT), $(addprefix $(BUILD_DIR)/, standard_library ) )
RUNTIME_OBJS = $(STDLIB_OBJS) $(RUNTIME_C_OBJS)

MAIN_RUNTIME_HEADERS = common coroutine io_commands semaphore
RUNTIME_HEADERS = $(addsuffix( .h, $(addprefix runtime_, $(MAIN_RUNTIME_HEADERS)) core )
APP_HEADERS = core.h charcoal_source_pre_header.h charcoal_main_pre_header.h core.h core_application.crclh standard_library.crclh atomics_wrappers.h
STDLIB_HEADERS = $(addsuffix .crclh, lib_utils stdlib stdio semaphore )

all: copy_stdlib_headers copy_app_headers $(RUNTIME_OBJS) $(STDLIB_OBJS) log_conf | $(LIB_DIR)
	ar rcs $(LIB_DIR)/libcharcoal_sys.a $(RUNTIME_OBJS)

clean:
	rm -rf $(BUILD_DIR)/*
	rm -f $(addprefix $(INCLUDE_DIR)/, $(APP_HEADERS))
	rm -f $(LIB_DIR)/libcharcoal_sys.a
	rm -f $(ETC_DIR)/*
	mkdir $(BUILD_DIR)/$(STDLIB_DIR)

log_conf: $(ETC_DIR)/charcoal_log.conf

$(ETC_DIR)/%.conf : %.conf | $(ETC_DIR)
	cp $< $@

copy_app_headers: $(addprefix $(INCLUDE_DIR)/, $(APP_HEADERS)) | $(INCLUDE_DIR)

copy_stdlib_headers: $(addprefix $(INCLUDE_DIR)/, $(STDLIB_HEADERS)) | $(INCLUDE_DIR)

$(INCLUDE_DIR)/%h : %h
	cp $< $@

$(INCLUDE_DIR)/%crclh : $(STDLIB_DIR)/%crclh
	cp $< $@

$(BUILD_DIR)/%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

std_lib : $(BUILD_DIR)/charcoal_std_lib.o

runtime : $(BUILD_DIR)/charcoal_runtime.o

$(INCLUDE_DIR):
	mkdir -p $(INCLUDE_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(ETC_DIR):
	mkdir -p $(ETC_DIR)

include charcoal_post.mk

# Generic rule for building in the build directory
%: $(BUILD_DIR)/%
	echo "Built \"$@\" by building \"$<\""

