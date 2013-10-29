/**
* 
* @file
*
* @brief  Analyzer factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_types.h>
#include <devices/state.h>

namespace Module
{
  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state);
}
