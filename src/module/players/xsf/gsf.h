/**
 *
 * @file
 *
 * @brief  GSF chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/xsf_factory.h"

namespace Module::GSF
{
  XSF::Factory::Ptr CreateFactory();
}  // namespace Module::GSF
