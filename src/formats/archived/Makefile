library_name := formats_archived
dirs.root := ../../..
source_dirs := .

jumbo.name := $(library_name)

include $(dirs.root)/makefile.mak

ifdef tools.python
fsb_vorbis_headers.inc: $(wildcard vorbis_setup_blocks/*)
	$(tools.python) make_vorbis_setups.py vorbis_setup_blocks > $@
endif

