# _very_ incomplete

BUILD_DIR = ../Build
INSTALL_DIR = ../Install
LIB_DIR = $(INSTALL_DIR)/lib
ETC_DIR = $(INSTALL_DIR)/etc
INCLUDE_DIR = $(INSTALL_DIR)/include
INCLUDE_DIRS = -I. -I $(INCLUDE_DIR)
CFLAGS += -Wall -O1 -g
CIL_DIR = ../ThirdParty/cil/Charcoal-1.7.3

CRCL_DOT_H_EXT=.crcl.with.h.c
CRCL_CPP_EXT=.crcl.cpp.c
CIL_C_EXT=.crcl.cil.c
CIL_O_EXT=.crcl.cil.o

# charcoal_log.conf

RUNTIME_SOURCES = coroutine coroutine_main coroutine_threads io_commands semaphore
RUNTIME_C_OBJS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/runtime_, $(RUNTIME_SOURCES)))
RUNTIME_CRCL_OBJS = $(addsuffix $(CIL_O_EXT), $(addprefix $(BUILD_DIR)/, standard_library ) )
RUNTIME_OBJS = $(RUNTIME_CRCL_OBJS) $(RUNTIME_C_OBJS)

MAIN_RUNTIME_HEADERS = atomics common coroutine io_commands semaphore
RUNTIME_HEADERS = $(addsuffix( .h, $(addprefix runtime_, $(MAIN_RUNTIME_HEADERS)) core )
APP_HEADERS = application_pre_header.h core.h core_application.crclh standard_library.crclh

.PRECIOUS: %$(CRCL_DOT_H_EXT)

all: copy_app_headers $(RUNTIME_OBJS) log_conf | $(LIB_DIR)
	ar rcs $(LIB_DIR)/libcharcoal_sys.a $(RUNTIME_OBJS) $(BUILD_DIR)/charcoal_std_lib.o

# Prepend the Charcoal runtime headers
$(BUILD_DIR)/%$(CRCL_DOT_H_EXT): %.crcl
	echo "#define __CHARCOAL_INSTALL_DIR \"$(INSTALL_DIR)\"" > $@
	cat application_pre_header.h >> $@
	echo "#line 1 " \"$<\" >> $@
	cat $< >> $@

# Do C preprocesing.  Cil doesn't do this itself.
%$(CRCL_CPP_EXT): %$(CRCL_DOT_H_EXT)
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

# Invoke Cil to translate Charcoal to plain C.
%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	$(CIL_DIR)/bin/cilly.native --tr coroutinify --out $@ $<

%$(CIL_O_EXT): %$(CIL_C_EXT)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c -o $@ $<

# Generic rule for building in the build directory
%: $(BUILD_DIR)/%
	echo "Built \"$@\" by building \"$<\""

clean:
	rm -rf $(BUILD_DIR)/*
	rm -f $(addprefix $(INCLUDE_DIR)/, $(APP_HEADERS))
	rm -f $(LIB_DIR)/libcharcoal_sys.a
	rm -f $(ETC_DIR)/*

log_conf: $(ETC_DIR)/charcoal_log.conf | $(ETC_DIR)

$(ETC_DIR)/%.conf : %.conf
	cp $< $@

copy_app_headers: $(addprefix $(INCLUDE_DIR)/, $(APP_HEADERS)) | $(INCLUDE_DIR)

$(INCLUDE_DIR)/%h : %h
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
