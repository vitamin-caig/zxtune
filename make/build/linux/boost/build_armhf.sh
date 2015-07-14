TOOLCHAIN_PATH=/opt/armhf-linux
BOOST_VERSION=`sed -nr 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' index.html`
echo "Building boost version ${BOOST_VERSION} for linux-armhf"
PREFIX=/mnt/devel/zxtune/Build/boost-${BOOST_VERSION}-linux-armhf
mkdir -p ${PREFIX}
echo "using gcc : arm : arm-linux-gnueabihf-g++ ;" > tools/build/src/user-config.jam
LD_LIBRARY_PATH=/opt/lib32/lib:/opt/lib32/usr/lib \
PATH=${TOOLCHAIN_PATH}/bin:${PATH} \
./bjam --prefix=${PREFIX} toolset=gcc-arm link=static threading=multi target-os=linux --layout=system \
   cflags="-march=armv6 -mfpu=vfp -mfloat-abi=hard" \
   --without-mpi --without-python \
   install
