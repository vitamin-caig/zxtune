QT_VERSION=`ls -1 changes* | sed -nr 's/changes-(.*)/\1/p'`
cp -Rf ../linux-mipsel-g++ mkspecs/qws
cp -f ../qconfig-embedded.h src/corelib/global
echo Bulding version ${QT_VERSION}
PREFIX=/mnt/devel/zxtune/Build/qt-${QT_VERSION}-dingux-mipsel
mkdir -p ${PREFIX}
export PATH=/opt/mipsel-linux-uclibc/usr/bin:$PATH
./configure -prefix ${PREFIX} \
-embedded mips -xplatform qws/linux-mipsel-g++ -little-endian \
-opensource -release -static -fast -confirm-license \
-depths 16 \
-qt-decoration-styled -no-decoration-windows -no-decoration-default \
-qt-gfx-linuxfb -no-gfx-transformed -no-gfx-qvfb -no-gfx-vnc -no-gfx-multiscreen -no-gfx-directfb -no-gfx-qnx \
-qt-kbd-linuxinput -no-kbd-tty -no-kbd-qvfb -no-kbd-qnx \
-no-mouse-pc -no-mouse-linuxtp -no-mouse-linuxinput -no-mouse-tslib -no-mouse-qvfb -no-mouse-qnx \
-nomake demos -nomake examples -nomake docs -nomake translations \
-no-accessibility -no-sql-sqlite -no-qt3support -no-xmlpatterns -no-multimedia -no-audio-backend -no-phonon -no-phonon-backend -no-svg -no-webkit \
-no-javascript-jit -no-script -no-scripttools -no-declarative -no-gif -no-libtiff -no-libmng -no-libjpeg -no-openssl -no-nis -no-cups -no-iconv \
-no-dbus -no-gtkstyle -no-opengl -no-openvg -no-sm -no-xshape -no-xvideo -no-xsync -no-xinerama -no-xcursor -no-xfixes -no-xrandr -no-xrender -no-mitshm \
-no-fontconfig -no-xinput -no-xkb -no-glib && make -j `grep -c processor /proc/cpuinfo` && make install
