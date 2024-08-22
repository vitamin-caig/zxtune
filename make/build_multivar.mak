all_arch=$(subst :, ,$(arch))
all_minsdk=$(subst :, ,$(android.minsdk))

ifneq ($(arch),$(all_arch))
all: $(addprefix arch_,$(all_arch))

arch_%:
	$(info Build for arch $*)
	$(MAKE) arch=$* $(MAKECMDGOALS)

else ifneq ($(android.minsdk),$(all_minsdk))
all: $(addprefix minsdk_,$(all_minsdk))

minsdk_%: minsdk_common
	$(info Build for minsdk $*)
	$(MAKE) android.minsdk=$* $(MAKECMDGOALS)

# Since minsdk variants differ only by final linking result, build everything at start.
# This should be last step before real makefile to use deps target.
# See platforms/android.mak
minsdk_common:
	$(info Build common code)
	$(MAKE) android.minsdk= deps

else
$(error Nor arch neither minsdk variants)
endif
