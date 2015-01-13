# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

CC=clang

TARGETS=sequential event_handlers threads processes

all: $(addprefix $(BUILD_DIR)/, $(TARGETS))

$(BUILD_DIR)/sequential: sequential.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

$(BUILD_DIR)/event_handlers: event_handlers.c multi_dns_utils.c
	$(CC) -I. -o $@ $^ -luv

$(BUILD_DIR)/threads: threads.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

$(BUILD_DIR)/processes: processes.c multi_dns_utils.c
	$(CC) -I. -o $@ $^
