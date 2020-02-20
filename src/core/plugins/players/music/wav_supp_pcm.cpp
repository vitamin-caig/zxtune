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
//common includes
#include <contract.h>
#include <make_ptr.h>

namespace Module
{
namespace Wav
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
          return Sound::Sample(SampleTraits<Type>::ConvertChannel(data[0]), SampleTraits<Type>::ConvertChannel(data[1]));
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
    
    using ConvertFunction = Sound::Chunk(*)(const void*, std::size_t);
    
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
  }

  class PcmModel : public Model
  {
  public:
    PcmModel(Pcm::ConvertFunction func, const Properties& props)
      : Convert(func)
      , Data(props.Data)
      , Frequency(props.Frequency)
      , TotalSamples(Data->Size() / props.BlockSize)
      , TotalDuration(Time::Microseconds::FromRatio(TotalSamples, Frequency))
      , TotalFrames(std::max<uint_t>(1, TotalDuration.Divide<uint_t>(props.FrameDuration)))
      , SamplesPerFrame(TotalSamples / TotalFrames)
      , BytesPerFrame(props.BlockSize * SamplesPerFrame)
    {
    }

    uint_t GetFrequency() const override
    {
      return Frequency;
    }
    
    uint_t GetFramesCount() const override
    {
      return TotalFrames;
    }
    
    uint_t GetSamplesPerFrame() const override
    {
      return SamplesPerFrame;
    }
    
    Sound::Chunk RenderFrame(uint_t idx) const override
    {
      const auto start = static_cast<const uint8_t*>(Data->Start()) + BytesPerFrame * idx;
      return Convert(start, SamplesPerFrame);
    }
    
  private:
    const Pcm::ConvertFunction Convert;
    const Binary::Data::Ptr Data;
    const uint_t Frequency;
    const std::size_t TotalSamples;
    const Time::Microseconds TotalDuration;
    const uint_t TotalFrames;
    const std::size_t SamplesPerFrame;
    const std::size_t BytesPerFrame;
  };
  
  Model::Ptr CreatePcmModel(const Properties& props)
  {
    const auto func = Pcm::FindConvertFunction(props.Channels, props.Bits, props.BlockSize);
    Require(func);
    return MakePtr<PcmModel>(func, props);
  }
  
  Model::Ptr CreateFloatPcmModel(const Properties& props)
  {
    const auto func = props.Channels == 1 ? &Pcm::ConvertChunk<float, 1> : &Pcm::ConvertChunk<float, 2>;
    Require(func);
    return MakePtr<PcmModel>(func, props);
  }
}
}
