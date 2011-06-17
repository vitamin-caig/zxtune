include $(path_step)/make/platforms/linux.mak

#makepkg_cmd ?= (cd $(1) && find . -type f -print0 | xargs -0 tar -cvzf $(CURDIR)/$(2))
makepkg_cmd = (cd $(1)/.. && makepkg -c -p archlinux/PKGBUILD)

pkg_suffix = pkg.tar.xz
