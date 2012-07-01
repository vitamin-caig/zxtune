/*
Abstract:
  ALSA subsystem access functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_ALSA_H_DEFINED
#define SOUND_BACKENDS_ALSA_H_DEFINED

//common includes
#include <iterator.h>
#include <string_helpers.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    namespace ALSA
    {
      class Device
      {
      public:
        typedef boost::shared_ptr<const Device> Ptr;
        typedef ObjectIterator<Ptr> Iterator;
        virtual ~Device() {}

        virtual String Id() const = 0;
        virtual String Name() const = 0;
        virtual String CardName() const = 0;

        virtual StringArray Mixers() const = 0;
      };

      Device::Iterator::Ptr EnumerateDevices();
    }
  }
}

#endif //SOUND_BACKENDS_ALSA_H_DEFINED
