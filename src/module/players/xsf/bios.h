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

//library includes
#include <binary/data.h>

namespace Module
{
  namespace PSF
  {
    const Binary::Data& GetSCPH10000HeBios();
  }
}
