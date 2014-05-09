# Do C preprocesing.  Cil doesn't do this itself.
$(BUILD_DIR)/%$(CRCL_CPP_EXT): %.crcl
	$(CC) -E $(INCLUDE_DIRS) -o $@ -x c $<

# Invoke Cil to translate Charcoal to plain C.
%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	$(CIL_DIR)/bin/cilly.native --out $@ $<

%: $(BUILD_DIR)/%
	
