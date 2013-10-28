QT_VERSION=`ls -1 changes* | sed -nr 's/changes-(.*)/\1/p'`
ARCH=$1
PLATFORM=$2
echo "Bulding version ${QT_VERSION} for ${ARCH}"
PREFIX=/mnt/devel/zxtune/Build/qt-${QT_VERSION}-linux-${ARCH}
mkdir -p ${PREFIX}
TARGETS="-nomake demos -nomake examples -nomake docs -nomake translations -nomake assistant -nomake qdoc3 -nomake qtracereplay -nomake linguist -nomake qt3to4 -nomake qcollectiongenerator -nomake pixeltool"
VERSIONS="-opensource -release -static -fast -confirm-license"
FORMATS="-no-gif -no-libtiff -no-libmng -no-libjpeg -qt-libpng"
FEATURES="-no-accessibility -no-opengl -no-openvg -no-sql-sqlite -no-qt3support -no-openssl -no-nis -no-cups -no-qdbus -no-dbus -no-gtkstyle -no-glib \
-fontconfig -xrender -xrandr -xfixes -xshape"
PARTS="-no-webkit -no-javascript-jit -no-phonon -no-phonon-backend -no-multimedia -no-audio-backend -no-script -no-scripttools -no-declarative -no-declarative-debug -no-xmlpatterns"
./configure -platform ${PLATFORM} -prefix ${PREFIX} $TARGETS $STYLES $VERSIONS $FORMATS $FEATURES $PARTS && make -j `grep -c processor /proc/cpuinfo` && make install
