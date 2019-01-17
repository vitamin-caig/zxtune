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
#include "wav_supp.h"
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
      Looped = {};
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
        Resampler = Sound::CreateResampler(Tune->GetFrequency(), SoundParams->SoundFreq(), Target);
      }
    }
  private:
    const Model::Ptr Tune;
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
      , Info(CreateStreamInfo(Data->GetFramesCount()))
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
      : Properties(props)
      , Meta(props)
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetProperties(uint_t formatCode, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) override
    {
      FormatCode = formatCode;
      WavProperties.Frequency = frequency;
      WavProperties.Channels = channels;
      WavProperties.Bits = bits;
      WavProperties.BlockSize = blockSize;
    }
    
    void SetSamplesData(Binary::Container::Ptr data) override
    {
      //smart copy
      WavProperties.Data = Binary::CreateContainer(std::move(data));
    }
    
    void SetSamplesCountHint(uint_t count) override
    {
      WavProperties.SamplesCountHint = count;
    }

    void SetFrameDuration(Time::Microseconds frameDuration)
    {
      WavProperties.FrameDuration = frameDuration;
    }
    
    Model::Ptr GetResult()
    {
      if (!WavProperties.Data)
      {
        return Model::Ptr();
      }
      switch (FormatCode)
      {
      case Formats::Chiptune::Wav::Format::PCM:
        return CreatePcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::IEEE_FLOAT:
        return CreateFloatPcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::ADPCM:
        return CreateAdpcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::IMA_ADPCM:
        return CreateImaAdpcmModel(WavProperties);
      default:
        return Model::Ptr();
      }
    }
  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    uint_t FormatCode = ~0;
    Wav::Properties WavProperties;
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
            props.SetFramesParameters(data->GetSamplesPerFrame(), data->GetFrequency());
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
