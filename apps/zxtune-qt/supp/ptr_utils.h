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

// std includes
#include <memory>
#include <type_traits>
// qt includes
#include <QtCore/QPointer>

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
