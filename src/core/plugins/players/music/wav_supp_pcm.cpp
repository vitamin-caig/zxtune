/**
 *
 * @file
 *
 * @brief  WAV PCM player code
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/music/wav_supp.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// std includes
#include <array>

namespace Module::Wav
{
  namespace Pcm
  {
    static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
    static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
    static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound sample channels count");

    template<class Type>
    struct SampleTraits;

    template<>
    struct SampleTraits<uint8_t>
    {
      static Sound::Sample::Type ConvertChannel(uint8_t v)
      {
        return Sound::Sample::MAX * v / 255;
      }
    };

    template<>
    struct SampleTraits<int16_t>
    {
      static Sound::Sample::Type ConvertChannel(int16_t v)
      {
        return v;
      }
    };

    using Sample24 = std::array<uint8_t, 3>;

    template<>
    struct SampleTraits<Sample24>
    {
      static_assert(sizeof(Sample24) == 3, "Wrong layout");

      static Sound::Sample::Type ConvertChannel(Sample24 v)
      {
        return static_cast<int16_t>(256 * v[2] + v[1]);
      }
    };

    template<>
    struct SampleTraits<int32_t>
    {
      static Sound::Sample::Type ConvertChannel(int32_t v)
      {
        return v / 65536;
      }
    };

    template<>
    struct SampleTraits<float>
    {
      static_assert(sizeof(float) == 4, "Wrong layout");

      static Sound::Sample::Type ConvertChannel(float v)
      {
        return v * 32767;
      }
    };

    template<class Type, uint_t Channels>
    struct MultichannelSampleTraits
    {
      using UnderlyingType = std::array<Type, Channels>;
      static_assert(sizeof(UnderlyingType) == Channels * sizeof(Type), "Wrong layout");

      static Sound::Sample ConvertSample(UnderlyingType data)
      {
        if (Channels == 2)
        {
          return Sound::Sample(SampleTraits<Type>::ConvertChannel(data[0]),
                               SampleTraits<Type>::ConvertChannel(data[1]));
        }
        else
        {
          const auto val = SampleTraits<Type>::ConvertChannel(data[0]);
          return Sound::Sample(val, val);
        }
      }
    };

    template<class Type, uint_t Channels>
    Sound::Chunk ConvertChunk(const void* data, std::size_t countHint)
    {
      using Traits = MultichannelSampleTraits<Type, Channels>;
      const auto typedData = static_cast<const typename Traits::UnderlyingType*>(data);
      Sound::Chunk result(countHint);
      std::transform(typedData, typedData + countHint, result.data(), &Traits::ConvertSample);
      return result;
    }

    using ConvertFunction = Sound::Chunk (*)(const void*, std::size_t);

    ConvertFunction FindConvertFunction(uint_t channels, uint_t bits)
    {
      switch (channels * 256 + bits)
      {
      case 0x108:
        return &ConvertChunk<uint8_t, 1>;
      case 0x208:
        return &ConvertChunk<uint8_t, 2>;
      case 0x110:
        return &ConvertChunk<int16_t, 1>;
      case 0x210:
        return &ConvertChunk<int16_t, 2>;
      case 0x118:
        return &ConvertChunk<Sample24, 1>;
      case 0x218:
        return &ConvertChunk<Sample24, 2>;
      case 0x120:
        return &ConvertChunk<int32_t, 1>;
      case 0x220:
        return &ConvertChunk<int32_t, 2>;
      default:
        return nullptr;
      }
    }

    ConvertFunction FindConvertFunction(uint_t channels, uint_t bits, uint_t blocksize)
    {
      if (channels * bits / 8 == blocksize)
      {
        return FindConvertFunction(channels, bits);
      }
      else if (bits >= 8 && (channels == 1 || 0 == (blocksize % channels)))
      {
        return FindConvertFunction(channels, (bits + 7) & ~7);
      }
      else
      {
        return nullptr;
      }
    }
  }  // namespace Pcm

  class PcmModel : public BlockingModel
  {
  public:
    PcmModel(Properties props, Pcm::ConvertFunction func)
      : BlockingModel(std::move(props))
      , Convert(func)
    {}

    Sound::Chunk RenderNextFrame() override
    {
      if (const auto samples = std::min<uint_t>(Stream.GetRestSize() / Props.BlockSize, Props.Frequency / 5))
      {
        const auto range = Stream.ReadData(samples * Props.BlockSize);
        return Convert(range.Start(), samples);
      }
      else
      {
        return {};
      }
    }

  private:
    const Pcm::ConvertFunction Convert;
  };

  Model::Ptr CreatePcmModel(Properties props)
  {
    const auto func = Pcm::FindConvertFunction(props.Channels, props.Bits, props.BlockSize);
    Require(func);
    return MakePtr<PcmModel>(std::move(props), func);
  }

  Model::Ptr CreateFloatPcmModel(Properties props)
  {
    const auto func = props.Channels == 1 ? &Pcm::ConvertChunk<float, 1> : &Pcm::ConvertChunk<float, 2>;
    Require(func);
    return MakePtr<PcmModel>(std::move(props), func);
  }
}  // namespace Module::Wav
