library_name := zxtune_app_version
path_step := ../..

source_dirs := src

text_files := text/text

$(platform)_definitions += BUILD_VERSION=$(root.version)

include $(path_step)/makefile.mak
include $(path_step)/make/version.mak

.PHONY: version_name version_index

version_name:
		$(info $(root.version))

version_index:
		$(info $(root.version.index))
