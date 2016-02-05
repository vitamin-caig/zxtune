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

template<class T>
inline typename T::Ptr MakePtr()
{
  return typename T::Ptr(new T());
}

template<class T>
inline typename T::RWPtr MakeRWPtr()
{
  return typename T::RWPtr(new T());
}

//TODO: use universal references and variadic templates for c++11
template<class T, class P1>
inline typename T::Ptr MakePtr(const P1& p1)
{
  return typename T::Ptr(new T(p1));
}

template<class T, class P1>
inline typename T::RWPtr MakeRWPtr(const P1& p1)
{
  return typename T::RWPtr(new T(p1));
}

template<class T, class P1, class P2>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2)
{
  return typename T::Ptr(new T(p1, p2));
}

template<class T, class P1, class P2, class P3>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3)
{
  return typename T::Ptr(new T(p1, p2, p3));
}

template<class T, class P1, class P2, class P3, class P4>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4)
{
  return typename T::Ptr(new T(p1, p2, p3, p4));
}

template<class T, class P1, class P2, class P3, class P4, class P5>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4,
                        const P5& p5)
{
  return typename T::Ptr(new T(p1, p2, p3, p4, p5));
}

template<class T, class P1, class P2, class P3, class P4, class P5, class P6>
inline typename T::Ptr MakePtr(const P1& p1,
                        const P2& p2,
                        const P3& p3,
                        const P4& p4,
                        const P5& p5,
                        const P6& p6)
{
  return typename T::Ptr(new T(p1, p2, p3, p4, p5, p6));
}
