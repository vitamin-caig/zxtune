.PHONY: all arch_%

all: $(foreach ar,$(subst :, ,$(arch)),arch_$(ar))

arch_%:
	$(info Build for arch $*)
	$(MAKE) arch=$* $(MAKECMDGOALS)
