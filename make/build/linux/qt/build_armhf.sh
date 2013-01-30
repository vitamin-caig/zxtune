QT_VERSION=`ls -1 changes* | sed -nr 's/changes-(.*)/\1/p'`
ARCH=armhf
PLATFORM=linux-armhf-g++
echo "Bulding version ${QT_VERSION} for ${ARCH}"
PREFIX=/mnt/devel/zxtune/Build/qt-${QT_VERSION}-linux-${ARCH}
mkdir -p ${PREFIX}
TARGETS="-nomake demos -nomake examples -nomake docs -nomake translations -nomake assistant -nomake qdoc3 -nomake qtracereplay -nomake linguist -nomake qt3to4 -nomake qcollectiongenerator -nomake pixeltool"
VERSIONS="-opensource -release -static -fast -confirm-license"
FORMATS="-no-gif -no-libtiff -no-libmng -no-libjpeg -qt-libpng"
FEATURES="-no-accessibility -no-opengl -no-openvg -no-sql-sqlite -no-qt3support -no-openssl -no-nis -no-cups -no-qdbus -no-dbus -no-gtkstyle -no-glib \
-fontconfig -xrender -xrandr -xfixes -xshape -no-mmx -no-3dnow -no-sse -no-sse2 -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-neon"
PARTS="-no-webkit -no-javascript-jit -no-phonon -no-phonon-backend -no-multimedia -no-audio-backend -no-script -no-scripttools -no-declarative -no-declarative-debug -no-xmlpatterns"

mkdir -p mkspecs/${PLATFORM}
echo -e "#include \"../linux-g++/qplatformdefs.h\"\n" > mkspecs/${PLATFORM}/qplatformdefs.h
cat <<EOF > mkspecs/${PLATFORM}/qmake.conf
MAKEFILE_GENERATOR	= UNIX
TARGET_PLATFORM		= unix
TEMPLATE		= app
CONFIG			+= qt warn_on release incremental link_prl gdb_dwarf_index
QT			+= core gui
QMAKE_INCREMENTAL_STYLE = sublib

include(../common/linux.conf)
include(../common/gcc-base-unix.conf)
include(../common/g++-unix.conf)

CROSS_ROOT = /mnt/devel/zxtune/Build/root-linux-armhf

QMAKE_CXXFLAGS = -march=armv6 -mfpu=vfp -mfloat-abi=hard
QMAKE_CFLAGS = \$\${QMAKE_CXXFLAGS}

QMAKE_INCDIR_X11 = \$\${CROSS_ROOT}/usr/include \$\${CROSS_ROOT}/usr/include/freetype2
QMAKE_LIBDIR_X11 = \$\${CROSS_ROOT}/usr/lib/arm-linux-gnueabihf

EXECPREFIX = arm-linux-gnueabihf-

# modifications to g++.conf
QMAKE_CC                = \$\${EXECPREFIX}gcc
QMAKE_CXX               = \$\${EXECPREFIX}g++
QMAKE_LINK              = \$\${EXECPREFIX}g++
QMAKE_LINK_SHLIB        = \$\${EXECPREFIX}g++

# modifications to linux.conf
QMAKE_AR                = \$\${EXECPREFIX}ar cqs
QMAKE_OBJCOPY           = \$\${EXECPREFIX}objcopy
QMAKE_STRIP             = \$\${EXECPREFIX}strip

QMAKE_LFLAGS            = -Wl,-rpath-link,\$\${CROSS_ROOT}/lib -Wl,-rpath-link,\$\${QMAKE_LIBDIR_X11}

load(qt_config)
EOF

export PATH=/opt/armhf-linux/bin:$PATH
./configure -v -arch arm -xplatform ${PLATFORM} -prefix ${PREFIX} $TARGETS $STYLES $VERSIONS $FORMATS $FEATURES $PARTS && make -j `grep -c processor /proc/cpuinfo` && make install
