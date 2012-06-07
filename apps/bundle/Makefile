pkg_name = zxtune
path_step := ../..
include $(path_step)/make/default.mak

all install install_$(platform) install_debug clean:
	$(MAKE) -C ../xtractor $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune123 $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune-qt $(MAKECMDGOALS)

include $(path_step)/make/platforms/$(platform).mak
include $(path_step)/make/hosts/$(host).mak
include $(path_step)/make/package.mak
