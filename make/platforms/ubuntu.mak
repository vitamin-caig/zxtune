include $(path_step)/make/platforms/linux.mak

makepkg_cmd = (fakeroot dpkg-deb --build $(1) $(CURDIR)/$(2))

pkg_suffix = deb
