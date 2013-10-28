/**
*
* @file     parameters/serialize.h
* @brief    Parameters-related types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_SERIALIZE_H_DEFINED
#define PARAMETERS_SERIALIZE_H_DEFINED

//library includes
#include <strings/map.h>

namespace Parameters
{
  void Convert(const Strings::Map& map, class Visitor& visitor);
  void Convert(const class Accessor& ac, Strings::Map& strings);
}

#endif //PARAMETERS_SERIALIZE_H_DEFINED
