/**
 *
 * @file
 *
 * @brief  DAC sample implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <devices/dac/sample_factories.h>

#include <make_ptr.h>

#include <cmath>
#include <cstring>
#include <numeric>

namespace Devices::DAC
{
  inline Sound::Sample::Type ToSample(uint_t val)
  {
    return Sound::Sample::MID
           + val * (Sound::Sample::MAX - Sound::Sample::MID) / (Sound::Sample::MAX - Sound::Sample::MIN);
  }

  static const Sound::Sample::Type AYVolumeTab[] = {
      ToSample(0x0000), ToSample(0x0340), ToSample(0x04C0), ToSample(0x06F2), ToSample(0x0A44), ToSample(0x0F13),
      ToSample(0x1510), ToSample(0x227E), ToSample(0x289F), ToSample(0x414E), ToSample(0x5B21), ToSample(0x7258),
      ToSample(0x905E), ToSample(0xB550), ToSample(0xD7A0), ToSample(0xFFFF)};

  inline Sound::Sample::Type FromU8(uint8_t inSample)
  {
    static_assert(Sound::Sample::MID == 0, "Sample should be signed");
    static_assert(Sound::Sample::MAX == 32767, "Sample should be 16-bit");
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
}  // namespace Devices::DAC

namespace Devices::DAC
{
  class BaseSample : public Sample
  {
  public:
    BaseSample(Binary::View content, std::size_t loop)
      : Content(new uint8_t[content.Size()])
      , StartValue(Content.get())
      , SizeValue(content.Size())
      , LoopValue(loop)
    {
      std::memcpy(Content.get(), content.Start(), SizeValue);
    }

    std::size_t Size() const override
    {
      return SizeValue;
    }

    std::size_t Loop() const override
    {
      return LoopValue;
    }

  protected:
    const std::unique_ptr<uint8_t[]> Content;
    const uint8_t* const StartValue;
    const std::size_t SizeValue;
    const std::size_t LoopValue;
  };

  class U8Sample : public BaseSample
  {
  public:
    U8Sample(Binary::View content, std::size_t loop)
      : BaseSample(content, loop)
    {}

    Sound::Sample::Type Get(std::size_t pos) const override
    {
      return pos < SizeValue ? FromU8(StartValue[pos]) : Sound::Sample::MID;
    }
  };

  class U4Sample : public BaseSample
  {
  public:
    U4Sample(Binary::View content, std::size_t loop)
      : BaseSample(content, loop)
    {}

    Sound::Sample::Type Get(std::size_t pos) const override
    {
      return pos < SizeValue ? FromU4Lo(StartValue[pos]) : Sound::Sample::MID;
    }
  };

  class U4PackedSample : public BaseSample
  {
  public:
    U4PackedSample(Binary::View content, std::size_t loop)
      : BaseSample(content, loop)
    {}

    Sound::Sample::Type Get(std::size_t pos) const override
    {
      if (pos < SizeValue * 2)
      {
        const uint8_t val = StartValue[pos >> 1];
        return 0 != (pos & 1) ? FromU4Hi(val) : FromU4Lo(val);
      }
      else
      {
        return Sound::Sample::MID;
      }
    }

    std::size_t Size() const override
    {
      return SizeValue * 2;
    }
  };

  Sample::Ptr CreateU8Sample(Binary::View content, std::size_t loop)
  {
    return MakePtr<U8Sample>(content, loop);
  }

  Sample::Ptr CreateU4Sample(Binary::View content, std::size_t loop)
  {
    return MakePtr<U4Sample>(content, loop);
  }

  Sample::Ptr CreateU4PackedSample(Binary::View content, std::size_t loop)
  {
    return MakePtr<U4PackedSample>(content, loop);
  }
}  // namespace Devices::DAC
