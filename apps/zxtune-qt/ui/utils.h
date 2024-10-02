/**
 *
 * @file
 *
 * @brief Different utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// qt includes
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>

inline QString ToQString(const String& str)
{
#ifdef UNICODE
  return QString::fromStdWString(str);
#else
  return QString::fromUtf8(str.data(), static_cast<int>(str.size()));
#endif
}

inline QString ToQString(StringView str)
{
  return QString::fromUtf8(str.data(), static_cast<int>(str.size()));
}

inline String FromQString(const QString& str)
{
#ifdef UNICODE
  return str.toStdWString();
#else
  const QByteArray& raw = str.toUtf8();
  return raw.constData();
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

class AutoBlockSignal
{
public:
  explicit AutoBlockSignal(QObject& obj)
    : Obj(obj)
    , Previous(Obj.blockSignals(true))
  {}

  ~AutoBlockSignal()
  {
    Obj.blockSignals(Previous);
  }

private:
  QObject& Obj;
  const bool Previous;
};

#define REGISTER_METATYPE(A)                                                                                           \
  {                                                                                                                    \
    static AutoMetaTypeRegistrator<A> tmp(#A);                                                                         \
  }
