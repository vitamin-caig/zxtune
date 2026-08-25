/**
 *
 * @file
 *
 * @brief  Delegate factory that opens module by trying every registered player plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

namespace ZXTune
{
  // Delegates module creation to the set of registered player plugins
  Module::Factory::Ptr CreatePluginsDelegateFactory();
}  // namespace ZXTune
