/*
Abstract:
  TrDOS utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__

//common includes
#include <types.h>

namespace Container
{
  class File;
}

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3]);

  bool AreFilesMergeable(const Container::File& lh, const Container::File& rh);
}
#endif //__CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__
