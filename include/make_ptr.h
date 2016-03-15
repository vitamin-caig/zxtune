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

//boost includes
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>

namespace Detail
{
  template<class Ptr, class ObjType>
  inline Ptr MakePtr(ObjType* obj)
  {
    using namespace boost;
    typedef typename Ptr::element_type PtrType;
    BOOST_STATIC_ASSERT(is_same<typename remove_cv<PtrType>::type, ObjType>::value || is_polymorphic<PtrType>::value);
    return Ptr(static_cast<PtrType*>(obj));
  }
}

template<class T>
inline typename T::Ptr MakePtr()
{
  return Detail::MakePtr<typename T::Ptr>(new T());
}

template<class T>
inline typename T::RWPtr MakeRWPtr()
{
  return Detail::MakePtr<typename T::RWPtr>(new T());
}

//TODO: use universal references and variadic templates for c++11
template<class T, class P1>
inline typename T::Ptr MakePtr(const P1& p1)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1));
}

template<class T, class P1>
inline typename T::RWPtr MakeRWPtr(const P1& p1)
{
  return Detail::MakePtr<typename T::RWPtr>(new T(p1));
}

template<class T, class P1, class P2>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1, p2));
}

template<class T, class P1, class P2, class P3>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1, p2, p3));
}

template<class T, class P1, class P2, class P3, class P4>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1, p2, p3, p4));
}

template<class T, class P1, class P2, class P3, class P4, class P5>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4,
                        const P5& p5)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1, p2, p3, p4, p5));
}

template<class T, class P1, class P2, class P3, class P4, class P5, class P6>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4,
                        const P5& p5,
                        const P6& p6)
{
  return Detail::MakePtr<typename T::Ptr>(new T(p1, p2, p3, p4, p5, p6));
}
