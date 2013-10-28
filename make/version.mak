-include $(path_step)/version.mak

ifndef root.version
#for svn
update_sources:
	svn up $(path_step)

root.version = $(shell svnversion $(path_step))
root.version.index = $(subst M,,$(lastword $(subst :, ,$(root.version))))
ifeq ($(root.version),)
root.version = develop
root.version.index = 0
endif
endif
