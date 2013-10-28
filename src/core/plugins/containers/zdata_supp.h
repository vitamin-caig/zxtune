/*
Abstract:
  Zdata containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_CONTAINERS_ZDATA_SUPP_H_DEFINED
#define CORE_PLUGINS_CONTAINERS_ZDATA_SUPP_H_DEFINED

//library includes
#include <core/data_location.h>

namespace ZXTune
{
  DataLocation::Ptr BuildZdataContainer(const Binary::Data& content);
}

#endif //CORE_PLUGINS_CONTAINERS_ZDATA_SUPP_H_DEFINED
