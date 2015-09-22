# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

CFLAGS+= -O1 -g

TARGETS = spawn_pthread spawn_activities memory_limit_pthread memory_limit_activities

.SECONDARY:

clean:
	rm -rf $(addprefix $(BUILD_DIR)/, $(addsuffix *, $(TARGETS)))

$(BUILD_DIR)/spawn_pthread: spawn_pthread.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

$(BUILD_DIR)/memory_limit_pthread: memory_limit_pthread.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

include $(CRCL_ROOT)/Testing/cil_rules.mk

