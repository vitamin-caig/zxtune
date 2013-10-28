/*
Abstract:
  Win32 subsystem access functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_WIN32_H_DEFINED
#define SOUND_BACKENDS_WIN32_H_DEFINED

//common includes
#include <iterator.h>
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace Win32
  {
    class Device
    {
    public:
      typedef boost::shared_ptr<const Device> Ptr;
      typedef ObjectIterator<Ptr> Iterator;
      virtual ~Device() {}

      virtual int_t Id() const = 0;
      virtual String Name() const = 0;
    };

    Device::Iterator::Ptr EnumerateDevices();
  }
}

#endif //SOUND_BACKENDS_WIN32_H_DEFINED
