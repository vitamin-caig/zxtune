/**
* 
* @file
*
* @brief  Zdata containers support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/data_location.h>

namespace ZXTune
{
  DataLocation::Ptr BuildZdataContainer(const Binary::Data& content);
}
