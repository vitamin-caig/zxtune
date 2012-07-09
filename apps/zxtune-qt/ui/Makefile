library_name := ui_common
path_step := ../../..

source_files = format \
               state \
               tools/filedialog \
               tools/errordialog

#informational
ui_files += informational/aboutdialog \
            informational/componentsdialog \
            tools/errorswidget
#controls
ui_files += controls/playback_controls \
            controls/seek_controls \
            controls/status_control \
            controls/volume_control \
            controls/playback_options

moc_files += controls/analyzer_control controls/overlay_progress

#other
moc_files += tools/parameters_helpers

include_dirs = ..

include $(path_step)/makefile.mak
