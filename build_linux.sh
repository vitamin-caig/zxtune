#!/bin/sh

Binary=zxtune123
Platform=linux
Arch=`arch`

echo "Building ${Binary} for platform ${Platform}_${Arch}"

# checking for textator
textator --version > /dev/null || exit 1;

# get current build and vesion
Revision=`svn info | grep Revision | cut -f 2 -d " "`
Revision=`printf %04u $Revision`
svn st 2>/dev/null | grep -q "\w  " && Revision=${Revision}-devel
echo "Revision ${Revision}"

Suffix=${Revision}_${Platform}_${Arch}
TargetDir=Builds/Revision${Suffix}
echo "Target dir ${TargetDir}"

echo "Clearing"
rm -Rf ${TargetDir} bin/${Platform}/release lib/${Platform}/release obj/${Platform}/release || exit 1;

echo "Creating build dir"
mkdir -p ${TargetDir}

echo "Building"
make mode=release platform=${Platform} defines=ZXTUNE_VERSION=rev${Revision} -C apps/zxtune123 > ${TargetDir}/build.log || exit 1;

ZipFile=${TargetDir}/${Binary}_${Suffix}.zip
echo "Compressing ${ZipFile}"
zip -9Dj ${ZipFile} bin/${Platform}/release/${Binary} || exit 1;

echo "Copy additional files"
cp bin/${Platform}/release/${Binary}*.pdb ${TargetDir} || exit 1;

echo Done
