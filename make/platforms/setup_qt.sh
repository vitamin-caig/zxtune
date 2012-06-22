#!/bin/sh

BUILD_DIR=${BUILD_DIR-`pwd`/../Build}
if [ "z${QT_VERSION}" = "z" ]; then
  QT_VERSION=`cat ${BUILD_DIR}/qt-${Platform}-${Arch} 2>/dev/null`
fi
QT_DIR=${BUILD_DIR}/qt-${QT_VERSION}-${Platform}-${Arch}

# Detect QT
if [ -d "${QT_DIR}" ]; then
  echo Using static QT ver ${QT_VERSION}
  export STATIC_QT_PATH=${QT_DIR}
  QT_BINARIES=${QT_DIR}/bin
  if [ -e "${QT_BINARIES}" ]; then
    export PATH=${QT_BINARIES}:${PATH}
  else
    echo Error: static QT binaries are not found
    exit 1
  fi
else
  QT_HEADERS_DIR=`qmake -query QT_INSTALL_HEADERS 2> /dev/null`
  if [ "x${QT_HEADERS_DIR}" != "x" ]; then
    echo Using system QT headers in ${QT_HEADERS_DIR}
    export CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH}:${QT_HEADERS_DIR}"
  else
    echo Error: unable to detect path to QT headers
    exit 1
  fi
  QT_LIBS_DIR=`qmake -query QT_INSTALL_LIBS 2> /dev/null`
  if [ "x${QT_LIBS_DIR}" != "x" ]; then
    echo Using system QT libs in ${QT_LIBS_DIR}
    export LIBRARY_PATH="${LIBRARY_PATH}:${QT_LIBS_DIR}"
  else
    echo Error: unable to detect path to QT libs
    exit 1
  fi
fi
