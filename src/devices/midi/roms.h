/**
* 
* @file
*
* @brief  Access to ROM images required for MT32Emu library
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//boost includes
#include <boost/shared_ptr.hpp>
//3rdparty includes
#include <3rdparty/mt32emu/src/ROMInfo.h>

namespace Devices
{
namespace MIDI
{
  namespace Detail
  {
    boost::shared_ptr<const MT32Emu::ROMImage> GetControlROM();
    boost::shared_ptr<const MT32Emu::ROMImage> GetPCMROM();
  }
}
}
