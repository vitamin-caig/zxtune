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
  
  struct Accumulator
  {
    uint64_t Total = 0;
    uint_t Count = 0;
    uint_t Min = 0;
    uint_t Max = 0;

    void Add(uint_t val)
    {
      Total += val;
      if (Count++)
      {
        Min = std::min(Min, val);
        Max = std::max(Max, val);
      }
      else
      {
        Min = Max = val;
      }
    }
    
    const uint_t* SingleValue() const
    {
      return Min == Max ? &Min : nullptr;
    }
    
    uint_t Avg() const
    {
      return static_cast<uint_t>((Total + (Count / 2)) / Count);
    }
  };
  
  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;
    
    Accumulator Frequency;
    Accumulator Samples;
    std::vector<Formats::Chiptune::Mp3::Frame::DataLocation> Frames;
    Time::Microseconds Duration;
    Binary::Data::Ptr Content;
  };
  
  struct FrameSound
  {
    uint_t Frequency = 0;
    Sound::Chunk Data;

    FrameSound()
      : Data(MINIMP3_MAX_SAMPLES_PER_FRAME / 2)
    {
    }
    
    FrameSound(const FrameSound&) = delete;
    FrameSound& operator = (const FrameSound&) = delete;
    FrameSound(FrameSound&& rh) noexcept// = default
      : Frequency(rh.Frequency)
      , Data(std::move(rh.Data))
    {
    }
    
    Sound::Sample::Type* GetTarget()
    {
       return safe_ptr_cast<Sound::Sample::Type*>(Data.data());
    }
    
    void Finalize(uint_t resultSamples, const mp3dec_frame_info_t& info)
    {
      if (1 == info.channels)
      {
        const auto pcm = GetTarget();
        for (std::size_t idx = resultSamples; idx != 0; --idx)
        {
          const auto mono = pcm[idx - 1];
          Data[idx - 1] = Sound::Sample(mono, mono);
        }
      }
      Data.resize(resultSamples);
      Frequency = info.hz;
    }
  };
  
  class Mp3Tune
  {
  public:
    explicit Mp3Tune(Model::Ptr data)
      : Data(std::move(data))
    {
      Reset();
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
    }
    
    FrameSound RenderFrame(uint_t idx)
    {
      const uint_t RENDER_FRAMES_LOOKAHEAD = 4;
      const auto& frame = GetMergedFrame(idx, RENDER_FRAMES_LOOKAHEAD);
      FrameSound result;
      mp3dec_frame_info_t info;
      const auto resultSamples = ::mp3dec_decode_frame(&Decoder, static_cast<const uint8_t*>(Data->Content->Start()) + frame.Offset, int(frame.Size), result.GetTarget(), &info);
      if (!resultSamples)
      {
        Dbg("No samples for frame @0x%1$08x..0x%2$08x", frame.Offset, frame.Offset + frame.Size - 1);
      }
      result.Finalize(resultSamples, info);
      return result;
    }
    
    void Reset()
    {
      ::mp3dec_init(&Decoder);
    }
    
    void Seek(uint_t idx)
    {
      Reset();
      const uint_t SEEK_FRAMES_LOOKAHEAD = 8;
      auto frame = idx <= SEEK_FRAMES_LOOKAHEAD ? GetMergedFrame(0, idx) : GetMergedFrame(idx - SEEK_FRAMES_LOOKAHEAD, SEEK_FRAMES_LOOKAHEAD);
      while (frame.Size)
      {
        mp3dec_frame_info_t info;
        ::mp3dec_decode_frame(&Decoder, static_cast<const uint8_t*>(Data->Content->Start()) + frame.Offset, int(frame.Size), nullptr, &info);
        if (info.frame_bytes)
        {
          frame.Offset += info.frame_bytes;
          frame.Size -= info.frame_bytes;
        }
        else
        {
          Dbg("Failed to decode frame @0x%1$08x..0x%2$08x", frame.Offset, frame.Offset + frame.Size - 1);
        }
      }
    }
  private:
    Formats::Chiptune::Mp3::Frame::DataLocation GetMergedFrame(uint_t start, uint_t size) const
    {
      auto result = Data->Frames.at(start);
      const auto end = start + size;
      const auto limit = end < Data->Frames.size() ? Data->Frames[end].Offset : Data->Content->Size();
      result.Size = limit - result.Offset;
      return result;
    }
  private:
    const Model::Ptr Data;
    mp3dec_t Decoder;
  };
  
  class MultiFreqTargetsDispatcher
  {
  public:
    explicit MultiFreqTargetsDispatcher(Sound::Receiver::Ptr target)
      : Target(target)
      , TargetFreq()
    {
    }
    
    void SetTargetFreq(uint_t freq)
    {
      if (TargetFreq != freq)
      {
        Resamplers.clear();
      }
      TargetFreq = freq;
    }
    
    void Put(FrameSound frame)
    {
      if (!frame.Data.empty())
      {
        GetTarget(frame.Frequency).ApplyData(std::move(frame.Data));
      }
    }
    
  private:
    Sound::Receiver& GetTarget(uint_t freq)
    {
      if (freq == TargetFreq)
      {
        return *Target;
      }
      for (const auto& resampled : Resamplers)
      {
        if (freq == resampled.first)
        {
          return *resampled.second;
        }
      }
      const auto res = Sound::CreateResampler(freq, TargetFreq, Target);
      Resamplers.emplace_back(freq, res);
      return *res;
    }
  private:
    const Sound::Receiver::Ptr Target;
    uint_t TargetFreq;
    std::vector<std::pair<uint_t, Sound::Receiver::Ptr> > Resamplers;
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

        auto frame = Tune.RenderFrame(State->Frame());
        Analyzer->AddSoundData(frame.Data);
        Target.Put(std::move(frame));
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
        Target.SetTargetFreq(SoundParams->SoundFreq());
      }
    }
  private:
    Mp3Tune Tune;
    const StateIterator::Ptr Iterator;
    const Module::State::Ptr State;
    const Module::SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    MultiFreqTargetsDispatcher Target;
    Sound::LoopParameters Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr data, Parameters::Accessor::Ptr props)
      : Data(std::move(data))
      , Info(CreateStreamInfo(static_cast<uint_t>(Data->Frames.size())))
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
  
  static const auto MIN_DURATION = Time::Seconds(1);
  
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
      Data->Frames.push_back(frame.Location);
      Data->Frequency.Add(frame.Properties.Samplerate);
      Data->Samples.Add(frame.Properties.SamplesCount);
      Data->Duration += Time::Microseconds::FromRatio(frame.Properties.SamplesCount, frame.Properties.Samplerate);
    }
    
    void SetContent(Binary::Data::Ptr data)
    {
      Data->Content = std::move(data);
    }
    
    Model::Ptr GetResult()
    {
      if (Data->Duration < MIN_DURATION)
      {
        return Model::Ptr();
      }
      else
      {
        const auto freq = GetFrequency();
        if (const auto frameSamples = Data->Samples.SingleValue())
        {
          Properties.SetFramesParameters(*frameSamples, freq);
        }
        else
        {
          const auto avg = Data->Samples.Avg();
          Dbg("Use average frame samples %1%", avg);
          Properties.SetFramesParameters(avg, freq);
        }
        Data->Frames.shrink_to_fit();
        return Data;
      }
    }
  private:
    uint_t GetFrequency() const
    {
      if (const auto singleFreq = Data->Frequency.SingleValue())
      {
        return *singleFreq;
      }
      else
      {
        const auto avg = Data->Frequency.Avg();
        Dbg("Use average frequency %1%", avg);
        return avg;
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
            dataBuilder.SetContent(container);
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

#undef FILE_TAG
