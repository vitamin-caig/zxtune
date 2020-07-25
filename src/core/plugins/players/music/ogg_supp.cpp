/**
* 
* @file
*
* @brief  OGG support plugin
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
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/music/oggvorbis.h>
#include <module/players/analyzer.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
//3rdparty
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 2
#if BOOST_ENDIAN_BIG_BYTE
#define STB_VORBIS_BIG_ENDIAN
#endif
#include <3rdparty/stb/stb_vorbis.c>

#define FILE_TAG B064CB04

namespace Module
{
namespace Ogg
{
  const Debug::Stream Dbg("Core::OggSupp");
  
  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;
    
    uint_t Frequency = 0;
    uint_t TotalSamples = 0;
    uint_t FramesCount = 0;
    uint_t SamplesPerFrame = 0;
    Binary::Data::Ptr Content;
  };
  
  class OggTune
  {
  public:
    explicit OggTune(Model::Ptr data)
      : Data(std::move(data))
    {
      Reset();
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
    }
    
    uint_t GetFrequency() const
    {
      return Data->Frequency;
    }
    
    Sound::Chunk RenderFrame()
    {
      Sound::Chunk chunk(Data->SamplesPerFrame);
      const auto samples = ::stb_vorbis_get_samples_short_interleaved(Decoder.get(), Sound::Sample::CHANNELS, safe_ptr_cast<short*>(chunk.data()), int(Sound::Sample::CHANNELS * chunk.size()));
      chunk.resize(samples);
      return chunk;
    }
    
    void Reset()
    {
      int error = 0;
      const auto decoder = VorbisPtr( ::stb_vorbis_open_memory(static_cast<const uint8_t*>(Data->Content->Start()), int(Data->Content->Size()), &error, nullptr), &::stb_vorbis_close);
      if (!decoder)
      {
        throw MakeFormattedError(THIS_LINE, "Failed to create decoder. Error: %1%", error);
      }
      Decoder = decoder;
    }
    
    void Seek(uint_t frame)
    {
      const auto sample = int(Data->SamplesPerFrame) * frame;
      if (!::stb_vorbis_seek(Decoder.get(), sample))
      {
        throw Error(THIS_LINE, "Failed to seek");
      }
    }
  private:
    const Model::Ptr Data;
    using VorbisPtr = std::shared_ptr<stb_vorbis>;
    VorbisPtr Decoder;
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

    Module::State::Ptr GetState() const override
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

        auto frame = Tune.RenderFrame();
        Analyzer->AddSoundData(frame);
        Resampler->ApplyData(std::move(frame));
        Iterator->NextFrame(Looped);
        if (0 == State->Frame())
        {
          Tune.Seek(0);
        }
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      Tune.Reset();
      SoundParams.Reset();
      Iterator->Reset();
      Looped = {};
    }

    void SetPosition(uint_t frame) override
    {
      Tune.Seek(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Tune.GetFrequency(), SoundParams->SoundFreq(), Target);
      }
    }
  private:
    OggTune Tune;
    const StateIterator::Ptr Iterator;
    const Module::State::Ptr State;
    const Module::SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::Receiver::Ptr Resampler;
    Sound::LoopParameters Looped;
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
  
  class DataBuilder : public Formats::Chiptune::OggVorbis::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Data(MakeRWPtr<Model>())
      , Meta(props)
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetStreamId(uint32_t id) override {};
    void SetProperties(uint_t /*channels*/, uint_t frequency, uint_t /*blockSizeLo*/, uint_t /*blockSizeHi*/) override
    {
      Data->Frequency = frequency;
    }

    void SetSetup(Binary::View /*data*/) override {}
    
    void AddFrame(std::size_t /*offset*/, uint_t samples, Binary::View /*data*/) override
    {
      Data->TotalSamples += samples;
    }
    
    void SetFrameDuration(Time::Microseconds frameDuration)
    {
      const auto totalDuration = Time::Microseconds::FromRatio(Data->TotalSamples, Data->Frequency);
      Data->FramesCount = std::max<uint_t>(1, totalDuration.Divide<uint_t>(frameDuration));
      Data->SamplesPerFrame = Data->TotalSamples / Data->FramesCount;
    }
    
    void SetContent(Binary::Data::Ptr data)
    {
      Data->Content = std::move(data);
    }
    
    Model::Ptr GetResult()
    {
      if (Data->TotalSamples)
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
        if (const auto container = Formats::Chiptune::OggVorbis::Parse(rawData, dataBuilder))
        {
          if (const auto data = dataBuilder.GetResult())
          {
            props.SetSource(*container);
            dataBuilder.SetContent(container);
            dataBuilder.SetFrameDuration(Sound::GetFrameDuration(params));
            return MakePtr<Holder>(data, properties);
          }
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create OGG: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterOGGPlugin(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'O', 'G', 'G', 0};
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateOGGDecoder();
    const auto factory = MakePtr<Module::Ogg::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}

#undef FILE_TAG
