/**
* 
* @file
*
* @brief  Data location internal factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/data_location.h>

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData, const String& subPlugin, const String& subPath);
  DataLocation::Ptr CreateLocation(Binary::Container::Ptr data, const String& plugin, const String& path);
}
