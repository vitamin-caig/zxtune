/**
* 
* @file
*
* @brief  WAV support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/music/wav.h>
#include <module/players/analyzer.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/render_params.h>
#include <sound/resampler.h>

#define FILE_TAG 72CE1906

namespace Module
{
namespace Wav
{
  const Debug::Stream Dbg("Core::WavSupp");

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
  Sound::Chunk ConvertChunk(const void* data, uint_t offset, std::size_t count)
  {
    using Traits = MultichannelSampleTraits<Type, Channels>;
    const auto typedData = static_cast<const typename Traits::UnderlyingType*>(data) + offset;
    Sound::Chunk result(count);
    std::transform(typedData, typedData + count, &result.front(), &Traits::ConvertSample);
    return result;
  }
  
  using ConvertFunction = Sound::Chunk(*)(const void* data, uint_t offset, std::size_t count);

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
  
  ConvertFunction FindConvertFunction(uint_t formatCode, uint_t channels, uint_t bits, uint_t blocksize)
  {
    using namespace Formats::Chiptune::Wav;
    if (formatCode == Format::PCM)
    {
      return FindConvertFunction(channels, bits, blocksize);
    }
    else if (formatCode == Format::IEEE_FLOAT)
    {
      return channels == 1 ? &ConvertChunk<float, 1> : &ConvertChunk<float, 2>;
    }
    else
    {
      return nullptr;
    }
  }
  
  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;
    
    uint_t Frequency = 0;
    uint_t SampleSize = 0;
    uint_t FramesCount = 0;
    uint_t SamplesPerFrame = 0;
    Binary::Data::Ptr SamplesData;
    ConvertFunction Convert;
    
    Sound::Chunk RenderFrame(uint_t frame) const
    {
      return Convert(SamplesData->Start(), frame * SamplesPerFrame, SamplesPerFrame);
    }
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr data, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(std::move(data))
      , Iterator(std::move(iterator))
      , State(Iterator->GetStateObserver())
      , Analyzer(Module::CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Target(std::move(target))
      , Looped()
    {
      ApplyParameters();
    }

    TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Analyzer;
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        auto frame = Tune->RenderFrame(State->Frame());
        Analyzer->AddSoundData(frame);
        Resampler->ApplyData(std::move(frame));
        Iterator->NextFrame(Looped);
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      SoundParams.Reset();
      Iterator->Reset();
    }

    void SetPosition(uint_t frame) override
    {
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Tune->Frequency, SoundParams->SoundFreq(), Target);
      }
    }
  private:
    const Model::Ptr Tune;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    const Module::SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::Receiver::Ptr Resampler;
    bool Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr data, Parameters::Accessor::Ptr props)
      : Data(std::move(data))
      , Info(CreateStreamInfo(Data->FramesCount))
      , Properties(std::move(props))
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MakePtr<Renderer>(Data, Module::CreateStreamStateIterator(Info), target, params);
    }
  private:
    const Model::Ptr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class DataBuilder : public Formats::Chiptune::Wav::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Data(MakeRWPtr<Model>())
      , Properties(props)
      , Meta(props)
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetProperties(uint_t formatCode, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) override
    {
      Data->Frequency = frequency;
      Data->SampleSize = blockSize;
      Data->Convert = FindConvertFunction(formatCode, channels, bits, blockSize);
      Require(Data->Convert);
    }
    
    void SetSamplesData(Binary::Container::Ptr data) override
    {
      //smart copy
      Data->SamplesData = Binary::CreateContainer(std::move(data));
    }
    
    void SetSamplesCountHint(uint_t /*count*/) override
    {
    }

    void SetFrameDuration(Time::Microseconds frameDuration)
    {
      if (Data->SamplesData)
      {
        const auto totalSamples = Data->SamplesData->Size() / Data->SampleSize;
        const auto totalDuration = Time::Microseconds(totalSamples * frameDuration.PER_SECOND / Data->Frequency);
        Data->FramesCount = static_cast<uint_t>(totalDuration.Get() / frameDuration.Get());
        Data->SamplesPerFrame = totalSamples / Data->FramesCount;
      }
    }
    
    Model::Ptr GetResult()
    {
      if (Data->FramesCount)
      {
        return Data;
      }
      else
      {
        return Model::Ptr();
      }
    }
  private:
    const Model::RWPtr Data;
    PropertiesHelper& Properties;
    MetaProperties Meta;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::Wav::Parse(rawData, dataBuilder))
        {
          dataBuilder.SetFrameDuration(Sound::GetFrameDuration(params));
          if (const auto data = dataBuilder.GetResult())
          {
            props.SetSource(*container);
            return MakePtr<Holder>(data, properties);
          }
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create WAV: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterWAVPlugin(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'W', 'A', 'V', 0};
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateWAVDecoder();
    const auto factory = MakePtr<Module::Wav::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
