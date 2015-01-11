# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

CC=clang

TARGETS=sequential libuv pthread

all: $(addprefix $(BUILD_DIR)/, $(TARGETS))

$(BUILD_DIR)/sequential: sequential.c multi_dns_utils.c
	$(CC) -I. -o $@ $^

$(BUILD_DIR)/libuv: lib_uv.c multi_dns_utils.c
	$(CC) -I. -o $@ $^ -luv

$(BUILD_DIR)/pthread: pthread.c multi_dns_utils.c
	$(CC) -I. -o $@ $^
