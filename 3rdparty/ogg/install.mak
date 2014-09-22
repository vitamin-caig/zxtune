install_oggvorbis:
	$(call copyfile_cmd,$(path_step)/3rdparty/ogg/$(arch)/ogg.dll,$(DESTDIR))
	$(call copyfile_cmd,$(path_step)/3rdparty/vorbis/$(arch)/*.dll,$(DESTDIR))
