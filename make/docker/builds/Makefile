.SECONDEXPANSION:

image.%: Dockerfile.%
	docker build --tag zxtune/$*:latest -f $^ .

.PHONY: image.%

root=/build/zxtune
result ?= .
branch ?= $(shell git rev-parse --abbrev-ref HEAD)
target ?= package -C apps/bundle
opts ?= -e jumbo.enabled=1

build.%: image.%
	docker run -it --name $@ $(foreach dir,bin obj lib,--tmpfs ${root}/${dir}) ${opts} zxtune/$* --branch=${branch} ${target} && touch $@ || true
	docker container cp $@:${root}/Builds ${result}
	docker container rm $@
	rm $@

build.android: image.android
	cat $(android_config) | (docker run -i --name $@ $(foreach dir,bin obj lib pkg,--tmpfs ${root}/${dir}) ${opts} zxtune/android --branch=${branch} --append_from_stdin=local.properties publicBuild && touch $@ || true)
	docker container cp $@:${root}/Builds ${result}
	docker container rm $@
	rm $@

.PHONY: build.%
