#!/bin/bash

set -ue

resize-darwin() {
  sips -z ${1} ${1} ${2} --out ${3}
}

build-darwin() {
  iconutil -c icns ${1}
}

resize-linux-gnu() {
  convert ${2} -resize ${1}x${1} ${3}
}

build-linux-gnu() {
  png2icns ${2} ${1}/*
}

input=${1}
output=${2}.iconset
mkdir -p ${output}
for size in 16 32 128 256 512
do
  resize-${OSTYPE} ${size} ${input} ${output}/icon_${size}x${size}.png
done
build-${OSTYPE} ${output} ${2}.icns
rm -R ${output}
