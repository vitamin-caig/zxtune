/**
 *
 * @file
 *
 * @brief  Pointers-related functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <cstddef>
#include <type_traits>

//! @brief Performing safe pointer casting
template<class T, class F>
inline T safe_ptr_cast(F from)
{
  using namespace std;
  static_assert(is_pointer<F>::value, "Source type should be pointer");
  static_assert(is_pointer<T>::value, "Target type should be pointer");
  using InType = remove_pointer_t<F>;
  using RetType = remove_pointer_t<T>;
  if constexpr (is_same<remove_const_t<InType>, void>::value)
  {
    static_assert(alignof(RetType) == 1, "Unaligned access");
  }
  else
  {
    static_assert(alignof(InType) % alignof(RetType) == 0, "Unaligned access");
  }
  using MidType = conditional_t<is_const<RetType>::value, const void*, void*>;
  return static_cast<T>(static_cast<MidType>(from));
}

//! @brief Stub deleter for shared pointers
template<class T>
class NullDeleter
{
public:
  void operator()(T*) {}
};

template<class T>
inline typename T::Ptr MakeSingletonPointer(T& obj)
{
  return typename T::Ptr(&obj, NullDeleter<T>());
}
