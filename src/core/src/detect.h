/**
* 
* @file
*
* @brief  Internal interfaces
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_detect.h>

namespace Module
{
  std::size_t Detect(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, DetectCallback& callback);
}
