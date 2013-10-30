SHELL = cmd

makedir_cmd = if NOT EXIST $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rmdir_cmd = $(if $(1),if EXIST $(subst /,\,$(1)) rmdir /Q /S $(subst /,\,$(1)),)
rmfiles_cmd = $(if $(1),del /Q $(subst /,\,$(1)),)
showtime_cmd = echo %TIME%
copyfile_cmd = copy /y $(subst /,\,$(1) $(2))
makepkg_cmd = (cd $(subst /,\,$(1)) && $(tools.root)zip -9rD $(CURDIR)\$(subst /,\,$(2)) .)
pkg_suffix := zip
embed_file_cmd = copy /B $(subst /,\,$@ $(addprefix +,$(embedded_files)) $@)
