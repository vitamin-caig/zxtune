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
#include <devices/state.h>
#include <module/analyzer.h>

namespace Module
{
  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state);
}
