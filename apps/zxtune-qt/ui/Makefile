library_name := ui_common
path_step := ../../..

source_files = format \
               informational/aboutdialog \
               informational/componentsdialog \
               tools/filedialog \
               tools/errordialog

#controls
ui_files += controls/playback_controls \
            controls/seek_controls \
            controls/status_control \
            controls/volume_control \
            controls/playback_options

ui_headeronly := informational/aboutdialog informational/componentsdialog

moc_files += controls/analyzer_control controls/overlay_progress

#other
moc_files += tools/parameters_helpers

include_dirs = ..

include $(path_step)/makefile.mak
