# TODO: Header
# 
# 

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

CFLAGS+= -O2

TARGETS = spawn_pthread

.SECONDARY:

clean:
	rm -rf $(addprefix $(BUILD_DIR)/, $(TARGETS))

spawn_pthread: $(BUILD_DIR)/spawn_pthread

$(BUILD_DIR)/spawn_pthread: spawn_pthread.c
	$(CC) $(CFLAGS) -o $@ $< -pthread

include $(CRCL_ROOT)/Testing/cil_rules.mk
