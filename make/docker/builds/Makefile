.SECONDEXPANSION:

.PHONY: image.%

root=/build/zxtune
result ?= .
branch ?= $(shell git rev-parse --abbrev-ref HEAD)
target ?= package -C apps/bundle
opts := $(opts) -e jumbo.enabled=1

ifneq (${CCACHE_REMOTE_STORAGE},)
opts := $(opts) -e CCACHE_REMOTE_STORAGE=${CCACHE_REMOTE_STORAGE} -e CCACHE_REMOTE_ONLY=true -e tools.cxxwrapper=ccache -e tools.ccwrapper=ccache
endif

suffix = $(if $(arch),-$(arch),)

image.%: Dockerfile.%
	docker build --tag zxtune/$*$(suffix):latest $(if $(arch),--build-arg arch=$(arch),) -f $^ .

build.%: image.%
	docker run -it --name $@$(suffix) $(foreach dir,bin obj lib,--tmpfs ${root}/${dir}) ${opts} zxtune/$*$(suffix) --branch=${branch} ${target} && touch $@$(suffix) || true
	docker container cp $@$(suffix):${root}/Builds ${result}
	docker container rm $@$(suffix)
	rm $@$(suffix)

build.android: image.android
	cat $(android_config) | (docker run -i --name $@ $(foreach dir,bin obj lib pkg,--tmpfs ${root}/${dir}) ${opts} zxtune/android --branch=${branch} --append_from_stdin=local.properties publicBuild && touch $@ || true)
	docker container cp $@:${root}/Builds ${result}
	docker container rm $@
	rm $@

.PHONY: build.%
