/**
*
* @file
*
* @brief  Win32 subsystem access functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <iterator.h>
#include <types.h>

namespace Sound
{
  namespace Win32
  {
    class Device
    {
    public:
      typedef std::shared_ptr<const Device> Ptr;
      typedef ObjectIterator<Ptr> Iterator;
      virtual ~Device() = default;

      virtual int_t Id() const = 0;
      virtual String Name() const = 0;
    };

    Device::Iterator::Ptr EnumerateDevices();
  }
}
