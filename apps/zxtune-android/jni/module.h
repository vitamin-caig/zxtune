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
#include <binary/container.h>
#include <core/module_holder.h>

namespace Module
{
  typedef ObjectsStorage<Module::Holder::Ptr> Storage;

  Storage::HandleType Create(Binary::Container::Ptr data);
}

#endif //MODULE_H_DEFINED
