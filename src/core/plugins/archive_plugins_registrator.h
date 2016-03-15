/**
* 
* @file
*
* @brief  Archive plugins registrator interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "archive_plugin.h"
#include "registrator.h"

namespace ZXTune
{
  typedef PluginsRegistrator<ArchivePlugin> ArchivePluginsRegistrator;
}
