/**
 *
 * @file
 *
 * @brief  USF chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/xsf_factory.h"

namespace Module::USF
{
  XSF::Factory::Ptr CreateFactory();
}  // namespace Module::USF
