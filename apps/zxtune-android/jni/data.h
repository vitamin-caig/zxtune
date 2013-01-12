/*
Abstract:
  Data access interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DATA_H_DEFINED
#define DATA_H_DEFINED

//local includes
#include "storage.h"
//library includes
#include <binary/container.h>

namespace Data
{
  typedef ObjectsStorage<Binary::Container::Ptr> Storage;
}

#endif //DATA_H_DEFINED
