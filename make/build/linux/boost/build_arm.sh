TOOLCHAIN_PATH=/opt/arm-linux
BOOST_VERSION=`sed -nr 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' index.html`
echo "Building boost version ${BOOST_VERSION} for arm linux"
PREFIX=/mnt/devel/zxtune/Build/boost-${BOOST_VERSION}-linux-arm
mkdir -p ${PREFIX}
echo "using gcc : arm : arm-none-linux-gnueabi-g++ ;" > tools/build/v2/user-config.jam
LD_LIBRARY_PATH=/opt/lib32/lib:/opt/lib32/usr/lib \
PATH=${TOOLCHAIN_PATH}/bin:${PATH} \
./bjam --prefix=${PREFIX} toolset=gcc-arm link=static threading=multi target-os=linux --layout=tagged \
   --without-mpi --without-python \
   install
