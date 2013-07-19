/*
Abstract:
  Data location additional functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_LOCATION_H_DEFINED
#define CORE_PLUGINS_LOCATION_H_DEFINED

//library includes
#include <core/data_location.h>

namespace ZXTune
{
  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Binary::Container::Ptr subData, const String& subPlugin, const String& subPath);
  DataLocation::Ptr CreateLocation(Binary::Container::Ptr data, const String& plugin, const String& path);
}

#endif //CORE_PLUGINS_LOCATION_H_DEFINED
