/*
Abstract:
  Module container and related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_MODULE_OPEN_H_DEFINED
#define CORE_MODULE_OPEN_H_DEFINED

//library includes
#include <core/data_location.h>
#include <core/module_holder.h>

namespace ZXTune
{
  namespace Module
  {
    //! @param location Source data location
    //! @return Object is exists. No object elsewhere
    Holder::Ptr Open(DataLocation::Ptr location);
  }
}

#endif //CORE_MODULE_OPEN_H_DEFINED
