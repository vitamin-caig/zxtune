-include $(path_step)/version.mak

ifndef root.version
root.version = $(shell svnversion $(path_step))
ifeq ($(root.version),)
root.version = develop
endif
endif
