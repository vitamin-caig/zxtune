/**
* 
* @file
*
* @brief  Plugins enumerator interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "archive_plugin.h"
#include "enumerator.h"

namespace ZXTune
{
  typedef PluginsEnumerator<ArchivePlugin> ArchivePluginsEnumerator;
}
