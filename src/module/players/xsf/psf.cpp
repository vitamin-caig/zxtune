/**
*
* @file
*
* @brief  PSF chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "bios.h"
#include "psf.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <formats/chiptune/emulation/playstationsoundformat.h>
#include <formats/chiptune/emulation/portablesoundformat.h>
#include <math/numeric.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
#include <sound/sound_parameters.h>
//3rdparty
#include <3rdparty/he/Core/bios.h>
#include <3rdparty/he/Core/iop.h>
#include <3rdparty/he/Core/psx.h>
#include <3rdparty/he/Core/r3000.h>
#include <3rdparty/he/Core/spu.h>

namespace Module
{
namespace PSF
{
  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    uint_t RefreshRate = 0;
    uint32_t PC = 0;
    uint32_t GP = 0;
    uint32_t SP = 0;
    uint32_t StartAddress = 0;
    Dump RAM;
    Time::Milliseconds Duration;
    Time::Milliseconds Fadeout;
  };
  
  class HELibrary
  {
  private:
    HELibrary()
    {
      const auto& bios = GetSCPH10000HeBios();
      ::bios_set_embedded_image(bios.Start(), bios.Size());
      Require(0 == ::psx_init());
    }
    
  public:
    std::unique_ptr<uint8_t[]> CreatePSX(int version) const
    {
      std::unique_ptr<uint8_t[]> res(new uint8_t[::psx_get_state_size(version)]);
      ::psx_clear_state(res.get(), version);
      return res;
    }
    
    static const HELibrary& Instance()
    {
      static const HELibrary instance;
      return instance;
    }
  };

  class PSXEngine : public Module::Analyzer
  {
  public:
    using Ptr = std::shared_ptr<PSXEngine>;
  
    static const constexpr uint_t SOUND_RATE = 44100;

    explicit PSXEngine(const ModuleData& data)
      : Emu(HELibrary::Instance().CreatePSX(1))
    {
      ::psx_set_refresh(Emu.get(), data.RefreshRate);
      SetRAM(data.StartAddress, data.RAM.data(), data.RAM.size());
      SetRegisters(data.PC, data.SP);
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      Sound::Chunk result(samples);
      for (uint32_t doneSamples = 0; doneSamples < samples; )
      {
        uint32_t toRender = samples - doneSamples;
        const auto res = ::psx_execute(Emu.get(), 0x7fffffff, safe_ptr_cast<short int*>(&result[doneSamples]), &toRender, 0);
        Require(res >= 0);
        Require(toRender != 0);
        doneSamples += toRender;
      }
      return result;
    }

    std::vector<ChannelState> GetState() const override
    {
      //http://problemkaputt.de/psx-spx.htm#soundprocessingunitspu
      std::vector<ChannelState> result;
      result.reserve(24);
      const auto iop = ::psx_get_iop_state(Emu.get());
      const auto spu = ::iop_get_spu_state(iop);
      for (uint_t voice = 0; voice < 24; ++voice)
      {
        if (const auto pitch = static_cast<int16_t>(::spu_lh(spu, 0x1f801c04 + (voice << 4))))
        {
          const auto envVol = static_cast<int16_t>(::spu_lh(spu, 0x1f801c0c + (voice << 4)));
          if (envVol > 327)
          {
            ChannelState state;
            //0x1000 is for 44100Hz, assume it's C-8
            state.Band = pitch * 96 / 0x1000;
            state.Level = uint_t(envVol) * 100 / 32768;
            result.push_back(state);
          }
        }
      }
      return result;
    }
  private:
    void SetRAM(uint32_t startAddr, const void* src, std::size_t size)
    {
    	startAddr &= 0x1fffff;
      Require(startAddr >= 0x10000 && size <= 0x1f0000 && startAddr + size <= 0x200000);
      const auto iop = ::psx_get_iop_state(Emu.get());
      ::iop_upload_to_ram(iop, startAddr, src, size);
    }
    
    void SetRegisters(uint32_t pc, uint32_t sp)
    {
      const auto iop = ::psx_get_iop_state(Emu.get());
      const auto cpu = ::iop_get_r3000_state(iop);
      ::r3000_setreg(cpu, R3000_REG_PC, pc);
      ::r3000_setreg(cpu, R3000_REG_GEN + 29, sp);
    }
  private:
    const std::unique_ptr<uint8_t[]> Emu;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Data(std::move(data))
      , Iterator(std::move(iterator))
      , State(Iterator->GetStateObserver())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Target(std::move(target))
      , Engine(MakePtr<PSXEngine>(*Data))
      , Looped()
      , SamplesPerFrame(PSXEngine::SOUND_RATE / Data->RefreshRate)
    {
      ApplyParameters();
    }

    TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Engine;
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        Resampler->ApplyData(Engine->Render(SamplesPerFrame));
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
      Engine = MakePtr<PSXEngine>(*Data);
    }

    void SetPosition(uint_t frame) override
    {
      /*
      SeekTune(frame);
      Module::SeekIterator(*Iterator, frame);
      */
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(PSXEngine::SOUND_RATE, SoundParams->SoundFreq(), Target);
      }
    }

    /*
    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Tune->Reset();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Tune->Skip(delta * SamplesPerFrame);
      }
    }
    */
  private:
    const ModuleData::Ptr Data;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::Receiver::Ptr Resampler;
    PSXEngine::Ptr Engine;
    bool Looped;
    const uint_t SamplesPerFrame;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Info(std::move(info))
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
      return MakePtr<Renderer>(Tune, Module::CreateStreamStateIterator(Info), std::move(target), std::move(params));
    }
  private:
    const ModuleData::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  struct DispatchedFields
  {
    String Title;
    String Artist;
    String Game;
    String Comment;
    String Copyright;
    String Dumper;
    
    void DumpTitle(PropertiesHelper& props)
    {
      if (!Title.empty())
      {
        props.SetTitle(std::move(Title));
      }
      else if (!Game.empty())
      {
        props.SetTitle(std::move(Game));
      }
    }
    
    void DumpAuthor(PropertiesHelper& props)
    {
      if (!Artist.empty())
      {
        props.SetAuthor(std::move(Artist));
      }
      else if (!Copyright.empty())
      {
        props.SetAuthor(std::move(Copyright));
      }
      else if (!Dumper.empty())
      {
        props.SetAuthor(std::move(Dumper));
      }
    }
    
    void DumpComment(PropertiesHelper& props)
    {
      if (!Comment.empty())
      {
        props.SetComment(std::move(Comment));
      }
      else if (!Game.empty())
      {
        props.SetComment(std::move(Game));
      }
      else if (!Copyright.empty())
      {
        props.SetComment(std::move(Copyright));
      }
      else if (!Dumper.empty())
      {
        props.SetComment(std::move(Dumper));
      }
    }
    
    void DumpProgram(PropertiesHelper& props)
    {
      props.SetProgram(std::move(Game));
    }
  };
  
  class DataBuilder : public Formats::Chiptune::PortableSoundFormat::Builder,
                      private Formats::Chiptune::PlaystationSoundFormat::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Result(MakeRWPtr<ModuleData>())
    {
    }
    
    void SetVersion(uint_t ver) override
    {
      Require(ver == Formats::Chiptune::PlaystationSoundFormat::VERSION_ID);
    }

    void SetReservedSection(Binary::Container::Ptr /*blob*/) override
    {
    }
    
    void SetProgramSection(Binary::Container::Ptr blob) override
    {
      Formats::Chiptune::PlaystationSoundFormat::ParsePSXExe(*blob, *this);
    }

    void SetTitle(String title) override
    {
      Fields.Title = std::move(title);
    }
    
    void SetArtist(String artist) override
    {
      Fields.Artist = std::move(artist);
    }
    
    void SetGame(String game) override
    {
      Fields.Game = std::move(game);
    }
    
    void SetYear(String date) override
    {
      Properties.SetDate(std::move(date));
    }
    
    void SetGenre(String /*genre*/) override
    {
    }
    
    void SetComment(String comment) override
    {
      Fields.Comment = std::move(comment);
    }
    
    void SetCopyright(String copyright) override
    {
      Fields.Copyright = std::move(copyright);
    }
    
    void SetDumper(String dumper) override
    {
      Fields.Dumper = std::move(dumper);
    }
    
    void SetLength(Time::Milliseconds duration) override
    {
      IsLibrary = false;
      Result->Duration = duration;
    }
    
    void SetFade(Time::Milliseconds fade) override
    {
      IsLibrary = false;
      Result->Fadeout = fade;
    }
    
    void SetTag(String name, String value) override
    {
      if (name == "_refresh")
      {
        Result->RefreshRate = std::atoi(value.c_str());
      }
      /*
      else if (name == "volume")
      {
        Result->Gain = ParseGain(value);
      }
      */
    }

    void SetLibrary(uint_t /*num*/, String /*filename*/) override
    {
      throw std::exception();
    }
    
    ModuleData::Ptr CaptureResult()
    {
      Require(!IsLibrary);
      if (!Result->RefreshRate)
      {
        Result->RefreshRate = 60;//default to NTSC
      }
      Fields.DumpTitle(Properties);
      Fields.DumpAuthor(Properties);
      Fields.DumpComment(Properties);
      Fields.DumpProgram(Properties);
      return std::move(Result);
    }
  private:
    void SetRegisters(uint32_t pc, uint32_t gp) override
    {
      Result->PC = pc;
      Result->GP = gp;
    }

    void SetStackRegion(uint32_t head, uint32_t /*size*/) override
    {
      Result->SP = head;
    }
    
    void SetRegion(String /*region*/, uint_t fps) override
    {
      if (0 != fps)
      {
        Result->RefreshRate = fps;
      }
    }
    
    void SetTextSection(uint32_t address, const Binary::Data& content) override
    {
      Result->StartAddress = address;
      const auto data = static_cast<const uint8_t*>(content.Start());
      Result->RAM.assign(data, data + content.Size());
    }
  private:
    PropertiesHelper& Properties;
    ModuleData::RWPtr Result;
    bool IsLibrary = true;
    DispatchedFields Fields;
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
        if (const auto container = Formats::Chiptune::PortableSoundFormat::Parse(rawData, dataBuilder))
        {
          auto tune = dataBuilder.CaptureResult();
          props.SetSource(*container);
          props.SetFramesFrequency(tune->RefreshRate);
          const auto period = Time::GetPeriodForFrequency<Time::Milliseconds>(tune->RefreshRate);
          const uint_t frames = tune->Duration.Get() / period.Get();
          const Information::Ptr info = CreateStreamInfo(frames);
          return MakePtr<Holder>(std::move(tune), info, properties);
        }
      }
      catch (const std::exception& e)
      {
      }
      return Module::Holder::Ptr();
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
