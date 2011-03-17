/*
Abstract:
  TrDOS entries processing

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_TRDOS_PROCESS_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_TRDOS_PROCESS_H_DEFINED__

//local includes
#include "trdos_utils.h"
#include "core/src/core.h"

namespace TRDos
{
  void ProcessEntries(ZXTune::DataLocation::Ptr location, const ZXTune::Module::DetectCallback& callback, ZXTune::Plugin::Ptr plugin, const FilesSet& files);
}

#endif //__CORE_PLUGINS_CONTAINERS_TRDOS_PROCESS_H_DEFINED__
