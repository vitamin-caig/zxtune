/**
*
* @file
* @brief    Simple strings map typedef
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef STRINGS_MAP_H_DEFINED
#define STRINGS_MAP_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <map>

namespace Strings
{
  typedef std::map<String, String> Map;
}

#endif //STRINGS_MAP_H_DEFINED
