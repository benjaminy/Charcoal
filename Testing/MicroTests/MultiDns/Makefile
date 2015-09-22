# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

include $(CRCL_ROOT)/Testing/cil_rules.mk

# CC=clang

INCLUDE_DIRS += -I.

CFLAGS += -g -O1

TARGETS=sequential event_handlers threads processes coroutines

all: $(addprefix $(BUILD_DIR)/, $(TARGETS))

clean:
	rm $(addprefix $(BUILD_DIR)/, $(TARGETS))

$(BUILD_DIR)/sequential: sequential.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

$(BUILD_DIR)/event_handlers: event_handlers.c multi_dns_utils.c
	$(CC) -I. -o $@ $^ -luv

$(BUILD_DIR)/threads: threads.c multi_dns_utils.c
	$(CC) -I. -o $@ $^ -pthread

$(BUILD_DIR)/processes: processes.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

$(BUILD_DIR)/coroutines: coroutines.c pcoroutine.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

activities: $(BUILD_DIR)/activities

$(BUILD_DIR)/activities: multi_dns_utils.c $(BUILD_DIR)/activities$(CRCL_O_EXT)
	$(CC) -I. -o $@ $^  $(CRCL_RUNTIME) $(ZLOG_LIB) $(LIBUV_FLAGS)
