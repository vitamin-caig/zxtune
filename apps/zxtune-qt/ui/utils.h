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
//qt includes
#include <QtCore/QString>
#include <QtCore/QMetaType>

inline QString ToQString(const String& str)
{
#ifdef UNICODE
  return QString::fromStdWString(str);
#else
  return QString::fromStdString(str);
#endif
}

inline String FromQString(const QString& str)
{
#ifdef UNICODE
  return str.toStdWString();
#else
  return str.toStdString();
#endif
}

template<class T>
struct AutoMetaTypeRegistrator
{
  explicit AutoMetaTypeRegistrator(const char* typeStr)
  {
    qRegisterMetaType<T>(typeStr);
  }
};

#define REGISTER_METATYPE(A) {static AutoMetaTypeRegistrator<A> tmp(#A);}

#endif //ZXTUNE_QT_UTILS_H_DEFINED
