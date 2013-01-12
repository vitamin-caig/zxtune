/*
Abstract:
  Module access interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef MODULE_H_DEFINED
#define MODULE_H_DEFINED

//local includes
#include "storage.h"
//library includes
#include <core/module_holder.h>

namespace Module
{
  typedef ObjectsStorage<ZXTune::Module::Holder::Ptr> Storage;
}

#endif //MODULE_H_DEFINED
