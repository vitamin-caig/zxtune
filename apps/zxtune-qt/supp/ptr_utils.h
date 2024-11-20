/**
 *
 * @file
 *
 * @brief Pointer-related utils interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QPointer>

#include <memory>
#include <type_traits>

template<class T, std::enable_if_t<std::is_base_of_v<QObject, T>, T>* = nullptr>
auto toWeak(T* val)
{
  return QPointer<T>(val);
}

template<class T>
auto toWeak(std::shared_ptr<T> val)
{
  return std::weak_ptr<T>(val);
}
