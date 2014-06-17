# TODO: Header
# 
# 

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

TARGETS = hello_world one_activity activate_loop

.SECONDARY:

clean:
	rm -f $(BUILD_DIR)/*

include $(CRCL_ROOT)/Testing/cil_rules.mk

$(BUILD_DIR)/just_calling: just_calling.c
	$(CC) -O1 -g -o $@ $<

$(BUILD_DIR)/just_calling_coro: just_calling_coro.c
	$(CC) -O1 -g -o $@ $<

$(BUILD_DIR)/just_calling_coro2: just_calling_coro2.c
	$(CC) -O1 -g -o $@ $<

$(BUILD_DIR)/just_calling_coro3: just_calling_coro3.c
	$(CC) -O1 -g -o $@ $<

$(BUILD_DIR)/just_calling_coro4: just_calling_coro4.c
	$(CC) -O1 -g -o $@ $<

$(BUILD_DIR)/just_calling.class: just_calling.java
	javac -d $(BUILD_DIR) just_calling.java
