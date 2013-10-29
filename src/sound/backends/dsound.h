/**
*
* @file
*
* @brief  DirectSound subsystem access functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <iterator.h>
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace DirectSound
  {
    class Device
    {
    public:
      typedef boost::shared_ptr<const Device> Ptr;
      typedef ObjectIterator<Ptr> Iterator;
      virtual ~Device() {}

      virtual String Id() const = 0;
      virtual String Name() const = 0;
    };

    Device::Iterator::Ptr EnumerateDevices();
  }
}
