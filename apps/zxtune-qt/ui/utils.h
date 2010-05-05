/*
Abstract:
  Different utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_UTILS_H_DEFINED
#define ZXTUNE_QT_UTILS_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QString>

inline QString ToQString(const String& str)
{
  return QString(str.c_str());
}

inline String FromQString(const QString& str)
{
  String tmp(str.size(), '\0');
  std::transform(str.begin(), str.end(), tmp.begin(), boost::mem_fn(&QChar::toAscii));
  return tmp;
}

#endif //ZXTUNE_QT_UTILS_H_DEFINED
