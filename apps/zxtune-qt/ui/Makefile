library_name := ui_common
path_step := ../../..

source_files = format \
               tools/filedialog \
               tools/errordialog

#controls
ui_files += controls/playback_controls \
            controls/seek_controls \
            controls/status_control \
            controls/volume_control

moc_files += controls/analyzer_control

#informational
ui_files += informational/aboutdialog \
            informational/componentsdialog

#other
moc_files += tools/parameters_helpers

include_dirs = ..

include $(path_step)/makefile.mak
