# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

CFLAGS+= -O2

.SECONDARY:

clean:
	rm -f $(BUILD_DIR)/*

$(BUILD_DIR)/baseline_strcmp: baseline_strcmp.c
	$(CC) $(CFLAGS) -o $@ $<

include $(CRCL_ROOT)/Testing/cil_rules.mk
