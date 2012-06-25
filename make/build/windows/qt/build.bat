set TARGETS=-nomake demos -nomake examples -nomake docs -nomake translations -no-dsp -no-vcproj -no-incredibuild-xge -no-ltcg
set STYLES=-no-style-cde -no-style-motif -no-style-cleanlooks -no-style-plastique -qt-style-windows -qt-style-windowsxp -qt-style-windowsvista
set VERSIONS=-opensource -debug-and-release -static -fast -confirm-license
set FORMATS=-no-gif -no-libtiff -no-libmng -no-libjpeg -no-freetype
set FEATURES=-no-accessibility -no-native-gestures -no-opengl -no-openvg -no-sql-sqlite -no-qt3support -no-openssl
set PARTS=-no-webkit -no-dbus -no-qdbus -no-phonon -no-phonon-backend -no-multimedia -no-audio-backend -no-script -no-scripttools -no-declarative -no-xmlpatterns
configure.exe %TARGETS% %STYLES% %VERSIONS% %FORMATS% %FEATURES% %PARTS% -platform %1 %2 %3
