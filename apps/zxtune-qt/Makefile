binary_name := zxtune-qt
path_step := ../..
include $(path_step)/default.mak
source_files := qt_app 

#ui-related
source_files += ui/mainwindow ui/playbackcontrol ui/seekcontrol
ui_files := ui/main_form ui/playback_controls ui/seek_controls

libraries := zxtune_app_base

depends := apps/base

$(platform)_libraries := QtGui

include $(path_step)/makefile.mak
