#!/bin/sh

Binaries=$1
Platform=$2
Arch=$3
Formats=txt
Languages=en

echo "Building ${Binary} for platform ${Platform}_${Arch}"

# checking for textator or assume that texts are correct
textator --version > /dev/null 2>&1 || touch text/*.cpp text/*.h

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
rm -Rf ${TargetDir} lib/${Platform}/release obj/${Platform}/release || exit 1;

echo "Creating build dir"
mkdir -p ${TargetDir}

# adding additional platform properties if required
case ${Arch} in
    ppc | ppc64 | powerpc)
	test `grep cpu /proc/cpuinfo | uniq | cut -f 3 -d " "` = "altivec" && cxx_flags="-mabi=altivec -maltivec"
	test `grep cpu /proc/cpuinfo | uniq | cut -f 2 -d " " | sed -e 's/,//g'` = "PPC970MP" && cxx_flags="${cxx_flags} -mtune=970 -mcpu=970"
    ;;
    *)
    ;;
esac

for Binary in ${Binaries}
do
echo "Building ${Binary}"
rm -Rf bin/${Platform}/release
time make -j `grep processor /proc/cpuinfo | wc -l` mode=release \
     platform=${Platform} defines=ZXTUNE_VERSION=rev${Revision} cxx_flags="${cxx_flags}" \
     -C apps/${Binary} > ${TargetDir}/build_${Binary}.log 2>&1 || exit 1;

ZipFile=${TargetDir}/${Binary}_r${Suffix}.zip
echo "Compressing ${ZipFile}"
zip -9Djr ${ZipFile} bin/${Platform}/release apps/zxtune.conf -x "*.pdb" || exit 1;
DistFiles=apps/${Binary}/dist/${Platform}
test -e ${DistFiles}/files.lst && zip -9Djr ${ZipFile} `cat ${DistFiles}/files.lst`;
if test -e apps/${Binary}/dist/${Binary}.txt
then
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
fi

echo "Copy additional files"
cp bin/${Platform}/release/${Binary}*.pdb ${TargetDir} || exit 1;
done
echo Done
