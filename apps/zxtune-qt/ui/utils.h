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

#include "string_type.h"
#include "string_view.h"

#include <QtCore/QLatin1String>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>

inline auto ToQString(StringView str)
{
  return QString::fromUtf8(str.data(), static_cast<int>(str.size()));
}

inline auto ToLatin(StringView str)
{
  return QLatin1String{str.data(), static_cast<int>(str.size())};
}

inline auto FromQString(const QString& str)
{
  const auto& raw = str.toUtf8();
  return String{raw.constData(), static_cast<std::size_t>(raw.size())};
}

inline constexpr auto operator"" _latin(const char* txt, std::size_t size)
{
  return QLatin1String(txt, size);
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
