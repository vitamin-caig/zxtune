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
#include <formats/chiptune/emulation/playstation2soundformat.h>
#include <formats/chiptune/emulation/portablesoundformat.h>
#include <debug/log.h>
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
//std includes
#include <map>
//3rdparty includes
#include <3rdparty/he/Core/bios.h>
#include <3rdparty/he/Core/iop.h>
#include <3rdparty/he/Core/psx.h>
#include <3rdparty/he/Core/r3000.h>
#include <3rdparty/he/Core/spu.h>

namespace Module
{
namespace PSF
{
  const Debug::Stream Dbg("Module::PSFSupp");
  
  class VfsImage
  {
  public:
    using Ptr = std::shared_ptr<const VfsImage>;
    using RWPtr = std::shared_ptr<VfsImage>;
    
    void AddFile(const String& path, Binary::Container::Ptr content)
    {
      Files.emplace(ToUpper(path.c_str()), std::move(content));
    }
    
    void PrefetchAll()
    {
      for (auto& file : Files)
      {
        if (file.second)
        {
          Require(file.second->Start() != nullptr);
        }
      }
    }
    
    sint32 Read(const char* path, sint32 offset, char* buffer, sint32 length) const
    {
      if (PreloadFile(path))
      {
        if (!CachedData)
        {
          //empty file
          return 0;
        }
        else if (length == 0)
        {
          //getting file size
          return CachedData->Size();
        }
        else if (const auto part = CachedData->GetSubcontainer(offset, length))
        {
          std::memcpy(buffer, part->Start(), part->Size());
          return part->Size();
        }
        else 
        {
          return 0;
        }
      }
      return -1;
    }
  private:
    static String ToUpper(const char* path)
    {
      String res;
      res.reserve(256);
      while (*path)
      {
        res.push_back(std::toupper(*path));
        ++path;
      }
      return res;
    }
  
    bool PreloadFile(const char* path) const
    {
      if (CachedName != path)
      {
        const auto it = Files.find(ToUpper(path));
        if (it == Files.end())
        {
          return false;
        }
        CachedName = path;
        CachedData = it->second;
      }
      return true;
    }
  private:
    std::map<String, Binary::Container::Ptr> Files;
    mutable String CachedName;
    mutable Binary::Container::Ptr CachedData;
  };
  
  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    uint_t RefreshRate = 0;
    Time::Milliseconds Duration;
    Time::Milliseconds Fadeout;

    uint32_t PC = 0;
    uint32_t GP = 0;
    uint32_t SP = 0;
    uint32_t StartAddress = 0;
    Dump RAM;
    
    VfsImage::RWPtr Vfs;
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
 
  struct SpuTrait
  {
    uint_t Base;
    uint_t PitchReg;
    uint_t VolReg;
  };
  
  const SpuTrait SPU1 = {0x1f801c00, 0x4, 0xc};//mirrored at {0x1f900000, 0x4, 0xa} for PS2
  const SpuTrait SPU2 = {0x1f900400, 0x4, 0xa};
  
  class PSXEngine : public Module::Analyzer
  {
  public:
    using Ptr = std::shared_ptr<PSXEngine>;
  
    void Initialize(const ModuleData& data)
    {
      if (data.RAM.size())
      {
        Emu = HELibrary::Instance().CreatePSX(1);
        SetRAM(data.StartAddress, data.RAM.data(), data.RAM.size());
        SetRegisters(data.PC, data.SP);
        SoundFrequency = 44100;
        Spus.assign({SPU1});
      }
      else if (data.Vfs)
      {
        Emu = HELibrary::Instance().CreatePSX(2);
        SetupVfs(data.Vfs);
        SoundFrequency = 48000;
        Spus.assign({SPU1, SPU2});
      }
      ::psx_set_refresh(Emu.get(), data.RefreshRate);
    }
    
    uint_t GetSoundFrequency() const
    {
      return SoundFrequency;
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
    
    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples; )
      {
        uint32_t toSkip = samples - skippedSamples;
        const auto res = ::psx_execute(Emu.get(), 0x7fffffff, nullptr, &toSkip, 0);
        Require(res >= 0);
        Require(toSkip != 0);
        skippedSamples += toSkip;
      }
    }

    std::vector<ChannelState> GetState() const override
    {
      //http://problemkaputt.de/psx-spx.htm#soundprocessingunitspu
      const uint_t SPU_VOICES_COUNT = 24;
      std::vector<ChannelState> result;
      result.reserve(SPU_VOICES_COUNT * Spus.size());
      const auto iop = ::psx_get_iop_state(Emu.get());
      const auto spu = ::iop_get_spu_state(iop);
      for (const auto& trait : Spus)
      {
        for (uint_t voice = 0; voice < SPU_VOICES_COUNT; ++voice)
        {
          const auto voiceBase = trait.Base + (voice << 4);
          if (const auto pitch = static_cast<int16_t>(::spu_lh(spu, voiceBase + trait.PitchReg)))
          {
            const auto envVol = static_cast<int16_t>(::spu_lh(spu, voiceBase + trait.VolReg));
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
    
    void SetupVfs(VfsImage::Ptr vfs)
    {
      Vfs = std::move(vfs);
      ::psx_set_readfile(Emu.get(), &VfsReadCallback, const_cast<void*>(static_cast<const void*>(Vfs.get())));
    }
    
    static sint32 VfsReadCallback(void* context, const char* path, sint32 offset, char* buffer, sint32 length)
    {
      Dbg("Read %3% from %1%@%2%", path, offset, length);
      const auto vfs = static_cast<VfsImage*>(context);
      return vfs->Read(path, offset, buffer, length);
    }
  private:
    uint_t SoundFrequency = 0;
    std::vector<SpuTrait> Spus;
    std::unique_ptr<uint8_t[]> Emu;
    VfsImage::Ptr Vfs;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Data(std::move(data))
      , Iterator(std::move(iterator))
      , State(Iterator->GetStateObserver())
      , Engine(MakePtr<PSXEngine>())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Target(std::move(target))
      , Looped()
    {
      Engine->Initialize(*Data);
      SamplesPerFrame = Engine->GetSoundFrequency() / Data->RefreshRate;
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
      Engine->Initialize(*Data);
    }

    void SetPosition(uint_t frame) override
    {
      SeekTune(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Engine->GetSoundFrequency(), SoundParams->SoundFreq(), Target);
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Engine->Initialize(*Data);
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine->Skip(delta * SamplesPerFrame);
      }
    }
  private:
    const ModuleData::Ptr Data;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    const PSXEngine::Ptr Engine;
    uint_t SamplesPerFrame;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::Receiver::Ptr Resampler;
    bool Looped;
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
  
  class DataBuilder : public Formats::Chiptune::PortableSoundFormat::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Result(MakeRWPtr<ModuleData>())
    {
    }
    
    void SetVersion(uint_t ver) override
    {
      Require(ver == Formats::Chiptune::PlaystationSoundFormat::VERSION_ID
           || ver == Formats::Chiptune::Playstation2SoundFormat::VERSION_ID);
    }

    void SetReservedSection(Binary::Container::Ptr blob) override
    {
      VFSParser parser(*Result);
      Formats::Chiptune::Playstation2SoundFormat::ParseVFS(*blob, parser);
    }
    
    void SetProgramSection(Binary::Container::Ptr blob) override
    {
      PSXExeParser parser(*Result);
      Formats::Chiptune::PlaystationSoundFormat::ParsePSXExe(*blob, parser);
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
      if (Result->Vfs)
      {
        Result->Vfs->PrefetchAll();
      }
      return std::move(Result);
    }
  private:
    class VFSParser : public Formats::Chiptune::Playstation2SoundFormat::Builder
    {
    public:
      explicit VFSParser(ModuleData& data)
      {
        if (!data.Vfs)
        {
          data.Vfs = MakeRWPtr<VfsImage>();
        }
        Vfs = data.Vfs.get();
      }

      void OnFile(String path, Binary::Container::Ptr content) override
      {
        Vfs->AddFile(path, std::move(content));
      }
    private:
      VfsImage* Vfs;
    };
    
    class PSXExeParser : public Formats::Chiptune::PlaystationSoundFormat::Builder
    {
    public:
      explicit PSXExeParser(ModuleData& data)
        : Result(data)
      {
      }

      void SetRegisters(uint32_t pc, uint32_t gp) override
      {
        Result.PC = pc;
        Result.GP = gp;
      }

      void SetStackRegion(uint32_t head, uint32_t /*size*/) override
      {
        Result.SP = head;
      }
      
      void SetRegion(String /*region*/, uint_t fps) override
      {
        if (0 != fps)
        {
          Result.RefreshRate = fps;
        }
      }
      
      void SetTextSection(uint32_t address, const Binary::Data& content) override
      {
        Result.StartAddress = address;
        const auto data = static_cast<const uint8_t*>(content.Start());
        Result.RAM.assign(data, data + content.Size());
      }
    private:
      ModuleData& Result;
    };
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
