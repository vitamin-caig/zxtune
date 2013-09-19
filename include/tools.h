/**
*
* @file     tools.h
* @brief    Commonly used tools
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef TOOLS_H_DEFINED
#define TOOLS_H_DEFINED

//std includes
#include <cstddef>

//! @brief Calculating size of fixed-size array
template<class T, std::size_t D>
inline std::size_t ArraySize(const T (&)[D])
{
  return D;
}

//! @fn template<class T, std::size_t D>inline const T* ArrayEnd(const T (&c)[D])
//! @brief Calculating end iterator of fixed-size array
template<class T, std::size_t D>
inline const T* ArrayEnd(const T (&c)[D])
{
  return c + D;
}

template<class T, std::size_t D>
inline T* ArrayEnd(T (&c)[D])
{
  return c + D;
}

#endif //TOOLS_H_DEFINED
