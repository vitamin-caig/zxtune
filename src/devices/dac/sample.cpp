/**
* 
* @file
*
* @brief  DAC sample implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <devices/dac/sample_factories.h>
//std includes
#include <cmath>
#include <numeric>

namespace
{
  const uint_t NO_RMS = uint_t(-1);

  inline Sound::Sample::Type ToSample(uint_t val)
  {
    return Sound::Sample::MID + val * (Sound::Sample::MAX - Sound::Sample::MID) / (Sound::Sample::MAX - Sound::Sample::MIN);
  }

  static const Sound::Sample::Type AYVolumeTab[] =
  {
    ToSample(0x0000), ToSample(0x0340), ToSample(0x04C0), ToSample(0x06F2),
    ToSample(0x0A44), ToSample(0x0F13), ToSample(0x1510), ToSample(0x227E),
    ToSample(0x289F), ToSample(0x414E), ToSample(0x5B21), ToSample(0x7258),
    ToSample(0x905E), ToSample(0xB550), ToSample(0xD7A0), ToSample(0xFFFF)
  };

  inline Sound::Sample::Type FromU8(uint8_t inSample)
  {
    BOOST_STATIC_ASSERT(Sound::Sample::MID == 0);
    BOOST_STATIC_ASSERT(Sound::Sample::MAX == 32767);
    return (Sound::Sample::Type(inSample) - 128) * 256;
  }

  inline Sound::Sample::Type FromU4Lo(uint8_t inSample)
  {
    return AYVolumeTab[inSample & 0x0f];
  }

  inline Sound::Sample::Type FromU4Hi(uint8_t inSample)
  {
    return AYVolumeTab[inSample >> 4];
  }
}

namespace Devices
{
  namespace DAC
  {
    class BaseSample : public Sample
    {
    public:
      BaseSample(Binary::Data::Ptr content, std::size_t loop)
        : Content(content)
        , StartValue(static_cast<const uint8_t*>(content->Start()))
        , SizeValue(content->Size())
        , LoopValue(loop)
        , RmsValue(NO_RMS)
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

      virtual uint_t Rms() const
      {
        if (RmsValue == NO_RMS)
        {
          uint64_t sum = 0;
          for (std::size_t idx = 0, lim = Size(); idx != lim; ++idx)
          {
            const int_t val = int_t(Get(idx)) - Sound::Sample::MID;
            sum += val * val;
          }
          RmsValue = static_cast<uint_t>(std::sqrt(float(sum) / Size()));
        }
        return RmsValue;
      }
    protected:
      const Binary::Data::Ptr Content;
      const uint8_t* const StartValue;
      const std::size_t SizeValue;
      const std::size_t LoopValue;
      mutable uint_t RmsValue;
    };
    
    class U8Sample : public BaseSample
    {
    public:
      U8Sample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual Sound::Sample::Type Get(std::size_t pos) const
      {
        return pos < SizeValue
          ? FromU8(StartValue[pos])
          : Sound::Sample::MID;
      }
    };

    class U4Sample : public BaseSample
    {
    public:
      U4Sample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual Sound::Sample::Type Get(std::size_t pos) const
      {
        return pos < SizeValue
          ? FromU4Lo(StartValue[pos])
          : Sound::Sample::MID;
      }
    };

    class U4PackedSample : public BaseSample
    {
    public:
      U4PackedSample(Binary::Data::Ptr content, std::size_t loop)
        : BaseSample(content, loop)
      {
      }

      virtual Sound::Sample::Type Get(std::size_t pos) const
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
          return Sound::Sample::MID;
        }
      }

      virtual std::size_t Size() const
      {
        return SizeValue * 2;
      }
    };

    Sample::Ptr CreateU8Sample(Binary::Data::Ptr content, std::size_t loop)
    {
      return MakePtr<U8Sample>(content, loop);
    }

    Sample::Ptr CreateU4Sample(Binary::Data::Ptr content, std::size_t loop)
    {
      return MakePtr<U4Sample>(content, loop);
    }

    Sample::Ptr CreateU4PackedSample(Binary::Data::Ptr content, std::size_t loop)
    {
      return MakePtr<U4PackedSample>(content, loop);
    }
  }
}
