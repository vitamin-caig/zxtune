install_lame:
	$(call copyfile_cmd,$(path_step)/3rdparty/lame/$(arch)/libmp3lame.dll,$(DESTDIR)/mp3lame.dll)
