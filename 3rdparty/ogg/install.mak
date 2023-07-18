install_oggvorbis:
	$(call copyfile_cmd,$(dirs.root)/3rdparty/ogg/$(arch)/*.dll,$(DESTDIR))
	$(call copyfile_cmd,$(dirs.root)/3rdparty/vorbis/$(arch)/*.dll,$(DESTDIR))
