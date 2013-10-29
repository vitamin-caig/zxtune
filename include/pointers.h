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

//std includes
#include <cstddef>
//boost includes
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>

//! @brief Performing safe pointer casting
template<class T, class F>
inline T safe_ptr_cast(F from)
{
  using namespace boost;
  BOOST_STATIC_ASSERT(is_pointer<F>::value);
  BOOST_STATIC_ASSERT(is_pointer<T>::value);
  typedef typename mpl::if_c<is_const<typename remove_pointer<T>::type>::value, const void*, void*>::type MidType;
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
