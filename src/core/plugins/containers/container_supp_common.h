/*
Abstract:
  Common container plugins support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__
#define __CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__

//local includes
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/archived.h>

namespace ZXTune
{
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder);

  String ProgressMessage(const String& id, const String& path);
  String ProgressMessage(const String& id, const String& path, const String& element);
}

#endif //__CORE_PLUGINS_CONTAINER_SUPP_COMMON_H_DEFINED__
