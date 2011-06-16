include $(path_step)/make/platforms/linux.mak

makepkg_cmd = (cd $(1) && find . -type f -print0 | xargs -0 tar -cvzf $(CURDIR)/$(2))
makepkg_cmd = (fakeroot dpkg-deb --build $(1) $(CURDIR)/$(2))

pkg_suffix = deb
