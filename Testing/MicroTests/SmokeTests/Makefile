# TODO: Header
#
#

CRCL_ROOT=../../..

include $(CRCL_ROOT)/Testing/generic.mk

TARGETS = hello_world one_activity activate_loop

.SECONDARY:

clean:
	rm -f $(BUILD_DIR)/*

hello_world_coroutine_manual: $(BUILD_DIR)/hello_world_coroutine_manual

$(BUILD_DIR)/hello_world_coroutine_manual: hello_world_coroutine_manual.c
	$(CC) -o $@ $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/hello_world_crcl: $(BUILD_DIR)/hello_world$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/hello_world_crcl $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/one_activity_crcl: $(BUILD_DIR)/one_activity$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/one_activity_crcl $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/activate_loop_crcl: $(BUILD_DIR)/activate_loop$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/activate_loop_crcl $(FLAGS) $< $(LIBUV_FLAGS)

$(BUILD_DIR)/activate_loop_ref_crcl: $(BUILD_DIR)/activate_loop_ref$(CIL_C_EXT)
	$(CC) -o $(BUILD_DIR)/activate_loop_ref_crcl $< $(FLAGS) $(LIBUV_FLAGS)

$(BUILD_DIR)/fake_crcl_hello_world: hello_world.c
	gcc -g -o $(BUILD_DIR)/hello_world $(FLAGS) -DYIELDING hello_world.c $(LIBUV_FLAGS)

one_activity: one_activity.c
	gcc -g -o one_activity $(FLAGS) -DYIELDING one_activity.c $(LIBUV_FLAGS)

activate_loop: activate_loop.c
	gcc -g -o activate_loop $(FLAGS) -DYIELDING activate_loop.c $(LIBUV_FLAGS)

include $(CRCL_ROOT)/Testing/cil_rules.mk
