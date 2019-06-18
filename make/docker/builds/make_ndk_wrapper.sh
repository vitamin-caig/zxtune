#!/bin/bash
triplet=$1
target=$2

ln -sT linux-x86_64 ${triplet}
prefix=${triplet}/bin/${triplet}-
for tool in gcc g++
do
  echo -e "#!/bin/bash\nif [ \"\$1\" != \"-cc1\" ]; then\nopt='-target ${target} -fno-addrsig'\nfi\n\$(dirname \${0})/clan${tool%%cc} \${opt} \"\$@\"" > ${prefix}${tool}
  chmod +x ${prefix}${tool}
done
