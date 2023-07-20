install_lame:
	$(call copyfile_cmd,$(dirs.root)/3rdparty/lame/$(arch)/*.dll,$(DESTDIR))
