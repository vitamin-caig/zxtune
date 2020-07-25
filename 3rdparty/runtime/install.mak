install_runtime:
	$(call copyfile_cmd,$(dirs.root)/3rdparty/runtime/$(platform)/$(arch)/*.dll,$(DESTDIR))
