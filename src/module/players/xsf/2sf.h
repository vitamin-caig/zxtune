/**
 *
 * @file
 *
 * @brief  2SF chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/xsf_factory.h"

namespace Module::TwoSF
{
  XSF::Factory::Ptr CreateFactory();
}  // namespace Module::TwoSF
