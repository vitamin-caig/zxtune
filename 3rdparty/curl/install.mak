install_curl:
	$(call copyfile_cmd,$(path_step)/3rdparty/curl/$(arch)/*.dll,$(DESTDIR))
