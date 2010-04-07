#!/bin/sh

Binary=$1
Platform=$2
Arch=$3
Formats=txt
Languages=en

echo "Building ${Binary} for platform ${Platform}_${Arch}"

# checking for textator
textator --version > /dev/null || exit 1;

# get current build and vesion
echo "Updating"
svn up > /dev/null || (echo "Failed to update"; exit 1)
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
time make -j `grep processor /proc/cpuinfo | wc -l` mode=release \
     platform=${Platform} defines=ZXTUNE_VERSION=rev${Revision} \
     -C apps/zxtune123 > ${TargetDir}/build.log || exit 1;

ZipFile=${TargetDir}/${Binary}_r${Suffix}.zip
echo "Compressing ${ZipFile}"
zip -9Djr ${ZipFile} bin/${Platform}/release apps/zxtune.conf -x "*.pdb" || exit 1;
DistFiles=apps/${Binary}/dist/${Platform}
test -e ${DistFiles}/files.lst && zip -9Djr ${ZipFile} `cat ${DistFiles}/files.lst`;
echo "Generating manuals"
for Fmt in $Formats
do
  for Lng in $Languages
  do
    textator --process --keys $Lng,$Fmt --asm \
    --output bin/${Binary}_${Lng}.${Fmt} apps/${Binary}/dist/${Binary}.txt || exit 1;
    zip -9Dj ${ZipFile} bin/${Binary}_${Lng}.${Fmt} || exit 1;
  done
done

echo "Copy additional files"
cp bin/${Platform}/release/${Binary}*.pdb ${TargetDir} || exit 1;

echo Done
