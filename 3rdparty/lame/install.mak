install_lame:
	$(call copyfile_cmd,$(dirs.root)/3rdparty/lame/$(arch)/libmp3lame.dll,$(DESTDIR)/mp3lame.dll)
