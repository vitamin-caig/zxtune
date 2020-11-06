/**
* 
* @file
*
* @brief  SPC support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <debug/log.h>
#include <devices/details/analysis_map.h>
#include <formats/chiptune/emulation/spc.h>
#include <math/numeric.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
//3rdparty
#include <3rdparty/snesspc/snes_spc/SNES_SPC.h>
#include <3rdparty/snesspc/snes_spc/SPC_Filter.h>
//text includes
#include <module/text/platforms.h>

namespace Module
{
namespace SPC
{
  const Debug::Stream Dbg("Core::SPCSupp");

  struct Model
  {
    using Ptr = std::shared_ptr<const Model>;

    const Dump Data;
    const Time::Milliseconds Duration;

    Model(const Binary::Data& data, Time::Milliseconds duration)
      : Data(static_cast<const uint8_t*>(data.Start()), static_cast<const uint8_t*>(data.Start()) + data.Size())
      , Duration(duration)
    {
    }
  };

  class SPC : public Module::Analyzer
  {
    static const uint_t SPC_DIVIDER = 1 << 12;
    static const uint_t C_7_FREQ = 2093;
  public:
    typedef std::shared_ptr<SPC> Ptr;
    
    explicit SPC(Binary::View data)
      : Data(data)
    {
      CheckError(Spc.init());
      // #0040 is C-1 (32Hz) - min
      // #0080 is C-2
      // #0100 is C-3
      // #0200 is C-4
      // #0400 is C-5
      // #0800 is C-6
      // #1000 is C-7 (2093Hz) - normal 32kHz
      // #2000 is C-8 (4186Hz)
      // #3fff is B-8 (7902Hz) - max
      const uint_t DIVIDER = ::SNES_SPC::sample_rate * SPC_DIVIDER / C_7_FREQ;
      Analysis.SetClockAndDivisor(::SNES_SPC::sample_rate, DIVIDER);
      Reset();
    }
    
    void Reset()
    {
      Spc.reset();
      CheckError(Spc.load_spc(Data.Start(), Data.Size()));
      Spc.clear_echo();
      Spc.disable_surround(true);
      Filter.clear();
      Filter.set_gain(::SPC_Filter::gain_unit * 1.4);//as in GME
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      Sound::Chunk result(samples);
      ::SNES_SPC::sample_t* const buffer = safe_ptr_cast< ::SNES_SPC::sample_t*>(result.data());
      CheckError(Spc.play(samples * Sound::Sample::CHANNELS, buffer));
      Filter.run(buffer, samples * Sound::Sample::CHANNELS);
      return result;
    }
    
    void Skip(uint_t samples)
    {
      CheckError(Spc.skip(samples * Sound::Sample::CHANNELS));
    }
    
    //http://wiki.superfamicom.org/snes/show/SPC700+Reference
    SpectrumState GetState() const override
    {
      const DspProperties dsp(Spc);
      const uint_t noise = dsp.GetNoiseChannels();
      const uint_t active = dsp.GetToneChannels() | noise;
      SpectrumState result;
      const uint_t noisePitch = noise != 0
        ? dsp.GetNoisePitch()
        : 0;
      for (uint_t chan = 0; active != 0 && chan != 8; ++chan)
      {
        const uint_t mask = (1 << chan);
        if (0 == (active & mask))
        {
          continue;
        }
        const uint_t levelInt = dsp.GetLevel(chan);
        const bool isNoise = 0 != (noise & mask);
        const uint_t pitch = isNoise
          ? noisePitch 
          : dsp.GetPitch(chan);
        const auto band = Analysis.GetBandByScaledFrequency(pitch);
        result.Set(band, LevelType(levelInt, 127));
      }
      return result;
    }
  private:
    inline static void CheckError(::blargg_err_t err)
    {
      Require(!err);//TODO: detalize
    }
    
    class DspProperties
    {
    public:
      explicit DspProperties(const ::SNES_SPC& spc)
        : Spc(spc)
      {
      }
      
      uint_t GetToneChannels() const
      {
        const uint_t keyOff = Spc.get_dsp_reg(0x5c);
        const uint_t endx = Spc.get_dsp_reg(0x7c);
        const uint_t echo = Spc.get_dsp_reg(0x4d);
        return ~(keyOff | endx) | echo;
      }
      
      uint_t GetNoiseChannels() const
      {
        return Spc.get_dsp_reg(0x3d);
      }
      
      uint_t GetNoisePitch() const
      {
        //absolute freqs based on 32kHz clocking
        static const uint_t NOISE_FREQS[32] =
        {
          0, 16, 21, 25, 31, 42, 50, 63,
          83, 100, 125, 167, 200, 250, 333, 400,
          500, 667, 800, 1000, 1300, 1600, 2000, 2700,
          3200, 4000, 5300, 6400, 8000, 10700, 16000, 32000
        };
        return NOISE_FREQS[Spc.get_dsp_reg(0x6c) & 0x1f] * SPC_DIVIDER / C_7_FREQ;
      }
      
      uint_t GetLevel(uint_t chan) const
      {
        const int_t volLeft = static_cast<int8_t>(Spc.get_dsp_reg(chan * 16 + 0));
        const int_t volRight = static_cast<int8_t>(Spc.get_dsp_reg(chan * 16 + 1));
        return std::max<uint_t>(Math::Absolute(volLeft), Math::Absolute(volRight));
      }
      
      uint_t GetPitch(uint_t chan) const
      {
        return (Spc.get_dsp_reg(chan * 16 + 3) & 0x3f) * 256 + Spc.get_dsp_reg(chan * 16 + 2);
      }
    private:
      const ::SNES_SPC& Spc;
    };
  private:
    const Binary::View Data;
    ::SNES_SPC Spc;
    ::SPC_Filter Filter;
    Devices::Details::AnalysisMap Analysis;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * ::SNES_SPC::sample_rate / period.PER_SECOND;
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, Sound::Converter::Ptr target)
      : Tune(std::move(tune))
      , Engine(MakePtr<SPC>(Tune->Data))
      , State(MakePtr<TimedState>(Tune->Duration))
      , Target(std::move(target))
    {
    }

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Engine;
    }

    Sound::Chunk Render(const Sound::LoopParameters& looped) override
    {
      if (!State->IsValid())
      {
        return {};
      }
      const auto avail = State->Consume(FRAME_DURATION, looped);
      return Target->Apply(Engine->Render(GetSamples(avail)));
    }

    void Reset() override
    {
      Engine->Reset();
      State->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine->Reset();
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine->Skip(GetSamples(toSkip));
      }
    }
  private:
    const Model::Ptr Tune;
    const SPC::Ptr Engine;
    const TimedState::Ptr State;
    const Sound::Converter::Ptr Target;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Tune, Sound::CreateResampler(::SNES_SPC::sample_rate, samplerate));
    }
  private:
    const Model::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::SPC::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
    {
    }

    void SetRegisters(uint16_t /*pc*/, uint8_t /*a*/, uint8_t /*x*/, uint8_t /*y*/, uint8_t /*psw*/, uint8_t /*sp*/) override
    {
    }

    void SetTitle(String title) override
    {
      if (Title.empty())
      {
        Properties.SetTitle(Title = std::move(title));
      }
    }
    
    void SetGame(String game) override
    {
      if (Program.empty())
      {
        Properties.SetProgram(Program = std::move(game));
      }
    }
    
    void SetDumper(String dumper) override
    {
      if (Author.empty())
      {
        Properties.SetAuthor(Author = std::move(dumper));
      }
    }
    
    void SetComment(String comment) override
    {
      if (Comment.empty())
      {
        Properties.SetComment(Comment = std::move(comment));
      }
    }
    
    void SetDumpDate(String date) override
    {
      Properties.SetDate(std::move(date));
    }
    
    void SetIntro(Time::Milliseconds duration) override
    {
      // Some tracks contain specified intro instead of loop
      if (!Loop)
      {
        Intro = duration;
        Properties.SetFadein(Intro);
      }
    }
    
    void SetLoop(Time::Milliseconds duration) override
    {
      Loop = duration;
    }
    
    void SetFade(Time::Milliseconds duration) override
    {
      if (duration)
      {
        Fade = duration;
        Properties.SetFadeout(duration);
      }
    }
    
    void SetArtist(String artist) override
    {
      Properties.SetAuthor(Author = std::move(artist));
    }
    
    void SetRAM(Binary::View /*data*/) override
    {
    }
    
    void SetDSPRegisters(Binary::View /*data*/) override
    {
    }
    
    void SetExtraRAM(Binary::View /*data*/) override
    {
    }
    
    Time::Milliseconds GetDuration() const
    {
      return Intro + Loop + Fade;
    }
  private:
    PropertiesHelper& Properties;
    String Title;
    String Program;
    String Author;
    String Comment;
    Time::Milliseconds Intro;
    Time::Milliseconds Loop;
    Time::Milliseconds Fade;
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
        if (const auto container = Formats::Chiptune::SPC::Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::SUPER_NINTENDO_ENTERTAINMENT_SYSTEM);

          auto duration = dataBuilder.GetDuration();
          if (!duration.Get())
          {
            duration = GetDefaultDuration(params);
          }
          auto tune = MakePtr<Model>(*container, duration);

          return MakePtr<Holder>(std::move(tune), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create SPC: %s", e.what());
      }
      return {};
    }
  };
}
}

namespace ZXTune
{
  void RegisterSPCSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'S', 'P', 'C', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::SPC700;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSPCDecoder();
    const Module::SPC::Factory::Ptr factory = MakePtr<Module::SPC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
