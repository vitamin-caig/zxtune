BOOST_VERSION=`sed -nr 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' index.html`
ARCH=$1
MODEL=$2
if [ -z "${ARCH}" -o -z "${MODEL}" ]; then
  echo Invalid parameters
  exit 1
fi
echo "Building boost version ${BOOST_VERSION} for ${ARCH}"
PREFIX=/mnt/devel/zxtune/Build/boost-${BOOST_VERSION}-linux-${ARCH}
mkdir -p ${PREFIX}
./bjam --prefix=${PREFIX} link=static threading=multi target-os=linux variant=release address-model=${MODEL} --layout=tagged \
--without-mpi --without-python boost.locale.icu=off install
