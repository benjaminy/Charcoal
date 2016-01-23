.PRECIOUS: %$(CRCL_DOT_H_EXT) %$(CRCL_CPP_EXT) %$(CIL_C_EXT) %$(CIL_CJ_EXT)

# Prepend the Charcoal runtime headers
$(BUILD_DIR)/%$(CRCL_DOT_H_EXT): %.crcl
	echo "#define __CHARCOAL_INSTALL_DIR \"$(INSTALL_DIR)\"" > $@
	cat $(INSTALL_DIR)/include/charcoal_source_pre_header.h >> $@
	echo "#line 1 " \"$<\" >> $@
	cat $< >> $@

# Do C preprocesing.  Cil doesn't do this itself.
%$(CRCL_CPP_EXT): %$(CRCL_DOT_H_EXT)
	$(CC) -E $(INCLUDE_DIRS) -D__CHARCOAL_CIL -o $@ -x c $<

# Invoke Cil to translate Charcoal to plain C.
%$(CIL_C_EXT): %$(CRCL_CPP_EXT)
	$(CIL_DIR)/bin/cilly.native --tr coroutinify --out $@ $<

# Prepend #include <setjmp.h>
%$(CRCL_CJ_EXT): %$(CIL_C_EXT)
	echo "#define jmp_buf __charcoal_jmp_buf_c" > $@
	echo "#include <setjmp.h>" >> $@
	echo "#undef jmp_buf" >> $@
	cat $< >> $@

# Build object code from a translated Charcoal file
%$(CRCL_O_EXT): %$(CRCL_CJ_EXT)
	$(CC) -c $(CFLAGS) -o $@ -x c $<
