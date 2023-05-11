binary_name := xtractor
dirs.root := ../..
source_dirs := .

source_files.windows := xtractor.rc
source_files.mingw := $(source_files.windows)

libraries.common = analysis async \
                   binary binary_compression binary_format \
                   debug \
                   formats_archived formats_chiptune formats_image formats_packed \
                   io \
                   l10n_stub \
                   parameters platform platform_application platform_version \
                   strings \
                   tools
libraries.3rdparty = lhasa lzma unrar zlib

libraries.boost = program_options
libraries.windows += advapi32 oldnames shell32

include $(dirs.root)/makefile.mak
include $(dirs.root)/make/package.mak

install_linux: install_linux_binary

install_linux_binary:
	install -D $(target) $(DESTDIR)/usr/bin/$(binary_name)

install_windows: install_mingw install_runtime

install_mingw:
	$(call copyfile_cmd,$(target),$(DESTDIR))

include $(dirs.root)/3rdparty/install.mak
