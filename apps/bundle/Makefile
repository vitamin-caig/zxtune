pkg_name = zxtune
dirs.root := ../..
include $(dirs.root)/make/default.mak

all install install_$(platform) install_debug install_linux_binary clean:
	$(MAKE) -C ../xtractor $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune123 $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune-qt $(MAKECMDGOALS)

include $(dirs.root)/make/environment.mak
include $(dirs.root)/make/platforms/$(platform).mak
include $(dirs.root)/make/hosts/$(host).mak
include $(dirs.root)/make/package.mak
