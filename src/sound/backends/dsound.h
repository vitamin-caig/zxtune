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

// common includes
#include <iterator.h>
#include <types.h>

namespace Sound
{
  namespace DirectSound
  {
    constexpr const Char BACKEND_ID[] = "dsound";

    class Device
    {
    public:
      typedef std::shared_ptr<const Device> Ptr;
      typedef ObjectIterator<Ptr> Iterator;
      virtual ~Device() = default;

      virtual String Id() const = 0;
      virtual String Name() const = 0;
    };

    Device::Iterator::Ptr EnumerateDevices();
  }  // namespace DirectSound
}  // namespace Sound
