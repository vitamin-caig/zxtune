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
#include <binary/data_view.h>

namespace Module
{
  namespace PSF
  {
    Binary::DataView GetSCPH10000HeBios();
  }
}
