/*
Abstract:
  DAC sample interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DAC_SAMPLE_H_DEFINED
#define DEVICES_DAC_SAMPLE_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Devices
{
  namespace DAC
  {
    typedef uint16_t SoundSample;
    const SoundSample SILENT = 0x8000;

    class Sample
    {
    public:
      typedef boost::shared_ptr<const Sample> Ptr;
      virtual ~Sample() {}

      virtual SoundSample Get(std::size_t pos) const = 0;
      virtual std::size_t Size() const = 0;
      virtual std::size_t Loop() const = 0;
      virtual SoundSample Average() const = 0;
    };
  }
}

#endif //DEVICES_DAC_SAMPLE_H_DEFINED
