QT_VERSION=`ls -1 changes* | sed -En 's/changes-(.*)/\1/p'`
ARCH=x86_64
PLATFORM=macx-g++
echo "Bulding version ${QT_VERSION} for ${ARCH}"
PREFIX=~/Develop/Build/qt-${QT_VERSION}-darwin-${ARCH}
mkdir -p ${PREFIX}
TARGETS="-nomake demos -nomake examples -nomake docs -nomake translations -nomake assistant -nomake qdoc3 -nomake qtracereplay -nomake linguist -nomake qt3to4 -nomake qcollectiongenerator -nomake pixeltool"
VERSIONS="-opensource -release -static -fast -confirm-license"
FORMATS="-no-gif -no-libtiff -no-libmng -no-libjpeg -qt-libpng -qt-zlib"
FEATURES="-no-accessibility -no-opengl -no-openvg -no-sql-sqlite -no-qt3support -no-openssl -no-nis -no-cups -no-dbus -no-framework"
PARTS="-no-webkit -no-javascript-jit -no-phonon -no-phonon-backend -no-multimedia -no-audio-backend -no-script -no-scripttools -no-declarative -no-declarative-debug -no-xmlpatterns"
patch src/gui/painting/qpaintengine_mac.cpp qpaintengine_mac.cpp.patch
./configure -platform ${PLATFORM} -arch ${ARCH} -prefix ${PREFIX} $TARGETS $STYLES $VERSIONS $FORMATS $FEATURES $PARTS && \
  make -j `sysctl -n hw.ncpu` && \
  make install
