include $(dirs.root)/3rdparty/curl/install.mak
include $(dirs.root)/3rdparty/FLAC/install.mak
include $(dirs.root)/3rdparty/lame/install.mak
include $(dirs.root)/3rdparty/ogg/install.mak
include $(dirs.root)/3rdparty/runtime/install.mak

install_3rdparty: install_curl install_flac install_lame install_oggvorbis
