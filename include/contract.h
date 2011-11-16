/*
Abstract:
  Programming by contract functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CONTRACT_H_DEFINED
#define CONTRACT_H_DEFINED

//std includes
#include <exception>

inline void Require(bool requirement)
{
  if (!requirement)
  {
    throw std::exception();
  }
}

#endif //CONTRACT_H_DEFINED
