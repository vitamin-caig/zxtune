-include $(path_step)/version.mak

ifndef root.version
#for git
#ensure version will have format r${revision}-...
root.version = $(subst rr,r,r$(shell git describe --dirty=M))
#extract revision from r${revision}-${delta}-g${hash}${dirty}
root.version.index = $(subst M,,$(subst r,,$(firstword $(subst -, ,$(root.version)))))
ifeq ($(root.version),r)
root.version = develop
root.version.index = 0
endif
endif

.PHONY: version_name version_index

version_name:
	$(info $(root.version))

version_index:
	$(info $(root.version.index))
