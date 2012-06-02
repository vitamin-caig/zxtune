pkg_name = zxtune
path_step := ../..
no_debuginfo = 1
include $(path_step)/make/default.mak

all install install_$(platform) clean:
	$(MAKE) -C ../xtractor $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune123 $(MAKECMDGOALS)
	$(MAKE) -C ../zxtune-qt $(MAKECMDGOALS)

include $(path_step)/make/platforms/$(platform).mak
include $(path_step)/make/hosts/$(host).mak
include $(path_step)/make/package.mak

