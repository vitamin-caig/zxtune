/**
 *
 * @file
 *
 * @brief  NCSF chiptune factory
 *
 * @author liushuyu011@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/xsf/xsf_factory.h"

namespace Module::NCSF
{
  XSF::Factory::Ptr CreateFactory();
}  // namespace Module::NCSF
