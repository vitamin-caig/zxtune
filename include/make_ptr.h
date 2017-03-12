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

//std includes
#include <utility>

template<class T, class... P>
inline typename T::Ptr MakePtr(P&&... p)
{
  return typename T::Ptr(static_cast<typename std::remove_const<typename T::Ptr::element_type>::type*>(new T(std::forward<P>(p)...)));
}

template<class T, class... P>
inline typename T::RWPtr MakeRWPtr(P&&... p)
{
  return typename T::RWPtr(static_cast<typename std::remove_const<typename T::RWPtr::element_type>::type*>(new T(std::forward<P>(p)...)));
}
