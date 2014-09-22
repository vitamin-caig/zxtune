include $(path_step)/3rdparty/curl/install.mak
include $(path_step)/3rdparty/FLAC/install.mak
include $(path_step)/3rdparty/lame/install.mak
include $(path_step)/3rdparty/ogg/install.mak
include $(path_step)/3rdparty/runtime/install.mak

install_3rdparty: install_curl install_flac install_lame install_oggvorbis
