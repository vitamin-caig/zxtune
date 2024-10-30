makedir_cmd = mkdir -p $(1)
rmdir_cmd = rm -Rf $(1)
rmfiles_cmd = rm -f $(1)
showtime_cmd = date +"%x %X"
copyfile_cmd = cp -f $(1) $(2)
makepkg_cmd ?= (cd $(1) && find . -type f -print0 | xargs -0 tar -cv | gzip - -9 > $(CURDIR)/$(2))
pkg_suffix ?= tar.gz
embed_file_cmd ?= $(foreach file,$(embedded_files),$(tools.objcopy) --add-section .emb_$(basename $(notdir $(file)))=$(file) $@ && ) echo "All files embedded"
