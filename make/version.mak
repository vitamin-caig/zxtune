-include $(path_step)/version.mak

ifndef root.version
#for git
#ensure version will have format r${revision}-...
root.version = $(subst rr,r,r$(shell git describe --dirty=M))
#extract revision from r${revision}-${delta}-g${hash}${dirty}
root.version.index = $(subst r,,$(firstword $(subst -, ,$(root.version))))
ifeq ($(root.version),)
root.version = develop
root.version.index = 0
endif
endif
