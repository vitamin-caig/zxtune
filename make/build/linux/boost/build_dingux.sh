TOOLCHAIN_PATH=/opt/mipsel-linux-uclibc
BOOST_VERSION=`sed -nr 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' index.html`
echo "Building boost version ${BOOST_VERSION} for dingux"
PREFIX=/mnt/devel/zxtune/Build/boost-${BOOST_VERSION}-dingux-mipsel
mkdir -p ${PREFIX}
echo "using gcc : mipsel : mipsel-linux-g++ ;" > tools/build/v2/user-config.jam
LD_LIBRARY_PATH=/opt/lib32/lib:/opt/lib32/usr/lib \
PATH=${TOOLCHAIN_PATH}/usr/bin:${PATH} \
./bjam --prefix=${PREFIX} toolset=gcc-mipsel link=static threading=multi target-os=linux \
   cflags="--sysroot=${TOOLCHAIN_PATH} -B${TOOLCHAIN_PATH} -L${TOOLCHAIN_PATH}/usr/lib -mips32" \
   cxxflags="-D'get_nprocs()=(1)' -D'WCHAR_MIN=(0)' -D'WCHAR_MAX=((1<<8*sizeof(wchar_t))-1)' -D'BOOST_FILESYSTEM_VERSION=2'" \
   --without-mpi --without-python \
   install
