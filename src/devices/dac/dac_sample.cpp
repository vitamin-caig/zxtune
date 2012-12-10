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

  static const Devices::DAC::SoundSample AYVolumeTab[] =
  {
    0x0000, 0x0340, 0x04C0, 0x06F2,
    0x0A44, 0x0F13, 0x1510, 0x227E,
    0x289F, 0x414E, 0x5B21, 0x7258,
    0x905E, 0xB550, 0xD7A0, 0xFFFF,
  };
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

    inline SoundSample FromU4Lo(uint8_t inSample)
    {
      return AYVolumeTab[inSample & 0x0f];
    }

    inline SoundSample FromU4Hi(uint8_t inSample)
    {
      return AYVolumeTab[inSample >> 4];
    }

    class BaseSample : public Sample
    {
    public:
      BaseSample(Binary::Data::Ptr content, std::size_t loop)
        : Content(content)
        , StartValue(static_cast<const uint8_t*>(content->Start()))
        , SizeValue(content->Size())
        , LoopValue(loop)
        , GainValue(NO_GAIN)
      {
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
          uint_t avg = 0;
          for (std::size_t idx = 0, lim = Size(); idx != lim; ++idx)
          {
            avg += Get(idx);
          }
          GainValue = avg / Size();
        }
        return GainValue;
      }
    protected:
      const Binary::Data::Ptr Content;
      const uint8_t* const StartValue;
      const std::size_t SizeValue;
      const std::size_t LoopValue;
      mutable uint_t GainValue;
    };
    
    class U8Sample : public BaseSample
    {
    public:
      U8Sample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual SoundSample Get(std::size_t pos) const
      {
        return pos < SizeValue
          ? FromU8(StartValue[pos])
          : SILENT;
      }
    };

    class U4Sample : public BaseSample
    {
    public:
      U4Sample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual SoundSample Get(std::size_t pos) const
      {
        return pos < SizeValue
          ? FromU4Lo(StartValue[pos])
          : SILENT;
      }
    };

    class U4PackedSample : public BaseSample
    {
    public:
      U4PackedSample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual SoundSample Get(std::size_t pos) const
      {
        if (pos < SizeValue * 2)
        {
          const uint8_t val = StartValue[pos >> 1];
          return 0 != (pos & 1)
            ? FromU4Hi(val)
            : FromU4Lo(val);
        }
        else
        {
          return SILENT;
        }
      }

      virtual std::size_t Size() const
      {
        return SizeValue * 2;
      }
    };

    Sample::Ptr CreateU8Sample(Binary::Data::Ptr content, std::size_t loop)
    {
      return boost::make_shared<U8Sample>(content, loop);
    }

    Sample::Ptr CreateU4Sample(Binary::Data::Ptr content, std::size_t loop)
    {
      return boost::make_shared<U4Sample>(content, loop);
    }

    Sample::Ptr CreateU4PackedSample(Binary::Data::Ptr content, std::size_t loop)
    {
      return boost::make_shared<U4PackedSample>(content, loop);
    }
  }
}
