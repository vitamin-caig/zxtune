# getaddr.sh PC [arch]
pc=$1
arch=${2:-armeabi-v7a}
/opt/armeabi-linux-android/bin/arm-linux-androideabi-addr2line -C -f -e bin/android_${arch}/release/libzxtune.so.pdb ${pc}
