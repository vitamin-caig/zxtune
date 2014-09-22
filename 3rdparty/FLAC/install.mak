install_flac:
	$(call copyfile_cmd,$(path_step)/3rdparty/FLAC/$(arch)/FLAC.dll,$(DESTDIR))
