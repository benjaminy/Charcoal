$(BUILD_DIR)/%$(CRCL_CPP_EXT): %.crcl
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	$(CIL_DIR)/bin/cilly.native --out $@ $<

%: $(BUILD_DIR)/%
	
