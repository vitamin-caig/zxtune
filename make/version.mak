-include $(path_step)/version.mak

ifndef root.version
#for git
update_sources:

root.version = $(shell git describe --dirty=M)
root.version.index = $(firstword $(subst -, ,$(root.version)))
ifeq ($(root.version),)
root.version = develop
root.version.index = 0
endif
endif
