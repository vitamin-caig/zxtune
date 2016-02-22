BOOST_VERSION=`sed -En 's/.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' index.html`
echo "Building boost version ${BOOST_VERSION} for darwin"
PREFIX=~/Develop/Build/boost-${BOOST_VERSION}-darwin-x86_64
mkdir -p ${PREFIX}
./bjam --prefix=${PREFIX} toolset=clang-darwin link=static threading=multi target-os=darwin address-model=64 --layout=tagged \
   --without-mpi --without-python \
   install
