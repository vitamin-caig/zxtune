/**
 *
 * @file
 *
 * @brief  HighlyExperimental BIOS access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <binary/view.h>

namespace Module
{
  namespace PSF
  {
    Binary::View GetSCPH10000HeBios();
  }
}  // namespace Module
