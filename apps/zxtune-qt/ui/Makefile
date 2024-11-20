library_name := ui_common
dirs.root := ../../..

source_dirs := .

#informational
source_dirs += informational tools

ui_files += informational/aboutdialog \
            informational/componentsdialog \
            tools/errorswidget
#controls
source_dirs += controls

ui_files += controls/playback_controls \
            controls/seek_controls \
            controls/status_control \
            controls/volume_control \
            controls/playback_options

moc_files += controls/analyzer_control controls/overlay_progress

#other
moc_files += tools/parameters_helpers

ts_files := ui_common

include $(dirs.root)/makefile.mak
