/*
Abstract:
  DAC sample implementations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <devices/dac_sample_factories.h>
//std includes
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const uint_t NO_GAIN = uint_t(-1);
}

namespace Devices
{
  namespace DAC
  {
    inline SoundSample FromU8(uint8_t inSample)
    {
      //simply shift bits
      return SoundSample(inSample) << (8 * (sizeof(SoundSample) - sizeof(inSample)));
    }
    
    class U8Sample : public Sample
    {
    public:
      U8Sample(Binary::Data::Ptr content, std::size_t loop)
        : Content(content)
        , StartValue(static_cast<const uint8_t*>(content->Start()))
        , SizeValue(content->Size())
        , LoopValue(loop)
        , GainValue(NO_GAIN)
      {
      }

      virtual SoundSample Get(std::size_t pos) const
      {
        return pos < SizeValue
          ? FromU8(StartValue[pos])
          : SILENT;
      }

      virtual std::size_t Size() const
      {
        return SizeValue;
      }

      virtual std::size_t Loop() const
      {
        return LoopValue;
      }

      virtual SoundSample Average() const
      {
        if (GainValue != NO_GAIN)
        {
          const uint8_t avg = static_cast<uint8_t>(std::accumulate(StartValue, StartValue + SizeValue, uint_t(0)) / SizeValue);
          GainValue = FromU8(avg);
        }
        return GainValue;
      }
    private:
      const Binary::Data::Ptr Content;
      const uint8_t* const StartValue;
      const std::size_t SizeValue;
      const std::size_t LoopValue;
      mutable uint_t GainValue;
    };

    Sample::Ptr CreateU8Sample(Binary::Data::Ptr content, std::size_t loop)
    {
      return boost::make_shared<U8Sample>(content, loop);
    }
  }
}
