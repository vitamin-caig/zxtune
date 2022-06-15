.PHONY: all arch_%

all: $(foreach ar,$(subst :, ,$(multiarch)),arch_$(ar))

arch_%:
	$(info Build for arch $*)
	$(MAKE) multiarch= arch=$* $(if $(bins_dir),bins_dir=$(bins_dir)/$*,)$(MAKECMDGOALS)
