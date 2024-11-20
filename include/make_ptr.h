/**
 *
 * @file
 *
 * @brief  Pointers creating with controllable overhead
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <utility>

namespace Details
{
  template<class PtrType, class T, class... P>
  auto Make(P&&... p)
  {
    using NativePtrType = decltype(&*std::declval<PtrType>());
    return PtrType(static_cast<NativePtrType>(new T(std::forward<P>(p)...)));
  }
}  // namespace Details

template<class T, class... P>
auto MakePtr(P&&... p)
{
  return Details::Make<typename T::Ptr, T>(std::forward<P>(p)...);
}

template<class T, class... P>
auto MakeRWPtr(P&&... p)
{
  return Details::Make<typename T::RWPtr, T>(std::forward<P>(p)...);
}
