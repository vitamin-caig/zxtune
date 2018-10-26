/**
* 
* @file
*
* @brief  MP3 support plugin
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
#include <formats/chiptune/music/mp3.h>
#include <module/players/analyzer.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
//3rdparty
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#include <3rdparty/minimp3/minimp3.h>

#define FILE_TAG 04123EA8

namespace Module
{
namespace Mp3
{
  const Debug::Stream Dbg("Core::Mp3Supp");
  
  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;
    
    Formats::Chiptune::Mp3::Frame::SoundProperties Properties;
    std::vector<Formats::Chiptune::Mp3::Frame::DataLocation> Frames;
    Binary::Data::Ptr Content;
  };
  
  class Mp3Tune
  {
  public:
    explicit Mp3Tune(Model::Ptr data)
      : Data(std::move(data))
    {
      ::mp3dec_init(&Decoder);
    }
    
    uint_t GetSoundFrequency() const
    {
      return Data->Properties.Samplerate;
    }
    
    Sound::Chunk RenderFrame(uint_t idx)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      const auto& frame = Data->Frames.at(idx);
      const auto mono = Data->Properties.Mono;
      const auto samples = Data->Properties.SamplesCount;
      Sound::Chunk output(samples);
      mp3dec_frame_info_t info;
      const auto pcmStereo = safe_ptr_cast<Sound::Sample::Type*>(output.data());
      const auto pcm = mono ? pcmStereo + samples : pcmStereo;
      const auto resultSamples = ::mp3dec_decode_frame(&Decoder, static_cast<const uint8_t*>(Data->Content->Start()) + frame.Offset, frame.Size, pcm, &info);
      if (int(frame.Size) != info.frame_bytes)
      {
        throw MakeFormattedError(THIS_LINE, "Frame size mismatch %1% -> %2%", frame.Size, info.frame_bytes);
      }
      else if (int(samples) != resultSamples)
      {
        throw MakeFormattedError(THIS_LINE, "Samples count mismatch %1% -> %2%", samples, resultSamples);
      }
      else if (2 - mono != info.channels)
      {
        throw MakeFormattedError(THIS_LINE, "Channels mismatch %1% -> %2%", 2 - mono, info.channels);
      }
      if (mono)
      {
        std::transform(pcm, pcm + samples, output.begin(), [](Sound::Sample::Type val) {return Sound::Sample(val, val);});
      }
      return output;
    }
    
  private:
    const Model::Ptr Data;
    mp3dec_t Decoder;
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

        auto buf = Tune.RenderFrame(State->Frame());
        Analyzer->AddSoundData(buf);
        Resampler->ApplyData(std::move(buf));
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
        Resampler = Sound::CreateResampler(Tune.GetSoundFrequency(), SoundParams->SoundFreq(), Target);
      }
    }
  private:
    Mp3Tune Tune;
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
      , Info(CreateStreamInfo(Data->Frames.size()))
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
  
  class DataBuilder : public Formats::Chiptune::Mp3::Builder
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

    void AddFrame(const Formats::Chiptune::Mp3::Frame& frame) override
    {
      if (Data->Frames.empty())
      {
        Data->Properties = frame.Properties;
        Properties.SetFramesParameters(frame.Properties.SamplesCount, frame.Properties.Samplerate);
      }
      //TODO: enable when variable frame duration will be possible
      if (Data->Properties.SamplesCount == frame.Properties.SamplesCount
       && Data->Properties.Samplerate == frame.Properties.Samplerate)
      {
        Data->Frames.push_back(frame.Location);
      }
      else
      {
        throw std::runtime_error("Variable frame parameters detected");
      }
    }
    
    void SetContent(const Binary::Data& data)
    {
      //copy
      Data->Content = Binary::CreateContainer(data.Start(), data.Size());
    }
    
    Model::Ptr GetResult()
    {
      if (Data->Frames.empty())
      {
        return Model::Ptr();
      }
      else
      {
        Data->Frames.shrink_to_fit();
        return Data;
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
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::Mp3::Parse(rawData, dataBuilder))
        {
          if (const auto data = dataBuilder.GetResult())
          {
            props.SetSource(*container);
            dataBuilder.SetContent(*container);
            return MakePtr<Holder>(data, properties);
          }
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create MP3: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterMP3Plugin(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'M', 'P', '3', 0};
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateMP3Decoder();
    const auto factory = MakePtr<Module::Mp3::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
