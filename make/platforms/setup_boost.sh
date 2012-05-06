#!/bin/sh

BUILD_DIR=${BUILD_DIR-`pwd`/../Build}
if [ "z${BOOST_VERSION}" = "z" ]; then
  BOOST_VERSION=`cat ${BUILD_DIR}/boost-${Platform}-${Arch}`
fi
BOOST_DIR=${BUILD_DIR}/boost-${BOOST_VERSION}-${Platform}-${Arch}

# Detect boost
if [ -d "${BOOST_DIR}" ]; then
  echo Using static boost ver ${BOOST_VERSION}
  export STATIC_BOOST_PATH=${BOOST_DIR}
else
  BOOST_DIR=/usr/include/boost
  if [ -d "${BOOST_DIR}" ]; then
    echo Using boost in ${BOOST_DIR}
  else
    echo Error: ${BOOST_DIR} does not exists
    exit 1
  fi
fi
