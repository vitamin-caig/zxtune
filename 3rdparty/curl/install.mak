install_curl:
	$(call copyfile_cmd,$(dirs.root)/3rdparty/curl/$(arch)/*.dll,$(DESTDIR))
