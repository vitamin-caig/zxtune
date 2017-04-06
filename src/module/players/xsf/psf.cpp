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
#include "psf.h"
#include "xsf.h"
#include "psf_bios.h"
#include "psf_exe.h"
#include "psf_vfs.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <module/additional_files.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/fading.h>
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

#define FILE_TAG 19ECDEC4

namespace Module
{
namespace PSF
{
  const Debug::Stream Dbg("Module::PSFSupp");
 
  class VfsIO
  {
  public:
    VfsIO() = default;
    explicit VfsIO(PsxVfs::Ptr vfs)
      : Vfs(std::move(vfs))
    {
    }
    
    sint32 Read(const char* path, sint32 offset, char* buffer, sint32 length)
    {
      if (PreloadFile(path))
      {
        if (length == 0)
        {
          const auto result = GetSize();
          Dbg("Size()=%1%", result);
          return result;
        }
        else
        {
          const auto result = Read(offset, buffer, length);
          Dbg("Read(%2%@%1%)=%3%", offset, length, result);
          return result;
        }
      }
      return -1;
    }
  private:
    bool PreloadFile(const char* path) const
    {
      if (CachedName != path)
      {
        if (!Vfs->Find(path, CachedData))
        {
          Dbg("Not found '%1%'", path);
          return false;
        }
        Dbg("Open '%1%'", path);
        CachedName = path;
      }
      return true;
    }
    
    sint32 GetSize() const
    {
      return CachedData ? CachedData->Size() : 0;
    }
    
    sint32 Read(sint32 offset, char* buffer, sint32 length) const
    {
      if (CachedData)
      {
        if (const auto part = CachedData->GetSubcontainer(offset, length))
        {
          std::memcpy(buffer, part->Start(), part->Size());
          return part->Size();
        }
      }
      return 0;
    }
  private:
    PsxVfs::Ptr Vfs;
    mutable String CachedName;
    mutable Binary::Container::Ptr CachedData;
  };
  
  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    uint_t Version = 0;
    PsxExe::Ptr Exe;
    PsxVfs::Ptr Vfs;
    XSF::MetaInformation::Ptr Meta;
    
    uint_t GetRefreshRate() const
    {
      if (Meta && Meta->RefreshRate)
      {
        return Meta->RefreshRate;
      }
      else if (Exe && Exe->RefreshRate)
      {
        return Exe->RefreshRate;
      }
      else
      {
        return 60;//NTSC by default
      }
    }
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
      if (data.Exe)
      {
        Emu = HELibrary::Instance().CreatePSX(1);
        SetupExe(*data.Exe);
        SoundFrequency = 44100;
        Spus.assign({SPU1});
      }
      else if (data.Vfs)
      {
        Emu = HELibrary::Instance().CreatePSX(2);
        SetupIo(data.Vfs);
        SoundFrequency = 48000;
        Spus.assign({SPU1, SPU2});
      }
      ::psx_set_refresh(Emu.get(), data.GetRefreshRate());
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
    void SetupExe(const PsxExe& exe)
    {
      SetRAM(exe.StartAddress, exe.RAM.data(), exe.RAM.size());
      SetRegisters(exe.PC, exe.SP);
    }
    
    void SetRAM(uint32_t startAddr, const void* src, std::size_t size)
    {
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
    
    void SetupIo(PsxVfs::Ptr vfs)
    {
      Io = VfsIO(vfs);
      ::psx_set_readfile(Emu.get(), &ReadCallback, &Io);
    }
    
    static sint32 ReadCallback(void* context, const char* path, sint32 offset, char* buffer, sint32 length)
    {
      const auto io = static_cast<VfsIO*>(context);
      return io->Read(path, offset, buffer, length);
    }
  private:
    uint_t SoundFrequency = 0;
    std::vector<SpuTrait> Spus;
    std::unique_ptr<uint8_t[]> Emu;
    VfsIO Io;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, Information::Ptr info, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Data(std::move(data))
      , Iterator(Module::CreateStreamStateIterator(info))
      , State(Iterator->GetStateObserver())
      , Engine(MakePtr<PSXEngine>())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , Looped()
    {
      Engine->Initialize(*Data);
      SamplesPerFrame = Engine->GetSoundFrequency() / Data->GetRefreshRate();
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
      return MakePtr<Renderer>(Tune, Info, std::move(target), std::move(params));
    }
    
    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties, Time::Seconds defaultDuration)
    {
      const auto period = Time::GetPeriodForFrequency<Time::Milliseconds>(tune->GetRefreshRate());
      const auto duration = tune->Meta && tune->Meta->Duration.Get() ? tune->Meta->Duration : Time::Milliseconds(defaultDuration);
      const uint_t frames = duration.Get() / period.Get();
      const Information::Ptr info = CreateStreamInfo(frames);
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      return MakePtr<Holder>(std::move(tune), info, std::move(properties));
    }
  private:
    const ModuleData::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class XsfView
  {
  public:
    explicit XsfView(const XSF::File& file)
      : File(file)
    {
    }
    
    bool IsSingleTrack() const
    {
      return File.Dependencies.empty();
    }
    
    bool IsMultiTrack() const
    {
      return !File.Dependencies.empty();
    }
    
    ModuleData::Ptr CreateModuleData() const
    {
      Require(File.Dependencies.empty());
      const ModuleData::RWPtr result = MakeRWPtr<ModuleData>();
      result->Version = File.Version;
      result->Meta = File.Meta;
      Require(!!File.ProgramSection != !!File.ReservedSection);
      if (File.ProgramSection)
      {
        auto exe = MakeRWPtr<PsxExe>();
        PsxExe::Parse(*File.ProgramSection, *exe);
        result->Exe = std::move(exe);
      }
      else
      {
        auto vfs = MakeRWPtr<PsxVfs>();
        PsxVfs::Parse(*File.ReservedSection, *vfs);
        vfs->Prefetch();
        result->Vfs = std::move(vfs);
      }
      return result;
    }
  private:
    const XSF::File& File;
  };
 
  class MultiFileHolder : public Module::Holder
                        , public Module::AdditionalFiles
  {
  public:
    MultiFileHolder(XSF::File head, Parameters::Container::Ptr properties, Time::Seconds defaultDuration)
      : DefaultDuration(defaultDuration)
      , Properties(std::move(properties))
      , Head(std::move(head))
    {
      CloneData(Head);
      LoadDependenciesFrom(Head);
    }
    
    Module::Information::Ptr GetModuleInformation() const override
    {
      return GetDelegate().GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return GetDelegate().GetModuleProperties();
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return GetDelegate().CreateRenderer(std::move(params), std::move(target));
    }
    
    Strings::Array Enumerate() const override
    {
      Strings::Array result;
      for (const auto& dep : Dependencies)
      {
        if (0 == dep.second.Version)
        {
          result.push_back(dep.first);
        }
      }
      return result;
    }
    
    void Resolve(const String& name, Binary::Container::Ptr data) override
    {
      XSF::File file;
      if (XSF::Parse(name, *data, file))
      {
        Dbg("Resolving dependency '%1%'", name);
        LoadDependenciesFrom(file);
        const auto it = Dependencies.find(name);
        Require(it != Dependencies.end() && 0 == it->second.Version);
        CloneData(file);
        it->second = std::move(file);
      }
    }
  private:
    static void CloneData(XSF::File& file)
    {
      CloneContainer(file.ProgramSection);
      CloneContainer(file.ReservedSection);
    }

    static void CloneContainer(Binary::Container::Ptr& data)
    {
      if (data)
      {
        data = Binary::CreateContainer(data->Start(), data->Size());
      }
    }

    void LoadDependenciesFrom(const XSF::File& file)
    {
      Require(Head.Version == file.Version);
      for (const auto& dep : file.Dependencies)
      {
        Require(!dep.empty());
        Require(Dependencies.emplace(dep, XSF::File()).second);
        Dbg("Found unresolved dependency '%1%'", dep);
      }
    }
    
    const Module::Holder& GetDelegate() const
    {
      if (!Delegate)
      {
        Require(!Dependencies.empty());
        auto mergedData = MergeDependencies();
        FillStrings();
        Delegate = PSF::Holder::Create(std::move(mergedData), std::move(Properties), DefaultDuration);
        Dependencies.clear();
        Head = XSF::File();
      }
      return *Delegate;
    }
    
    class ModuleDataBuilder
    {
    public:
      void AddExe(const Binary::Container& blob)
      {
        Require(!Vfs);
        if (!Exe)
        {
          Exe = MakeRWPtr<PsxExe>();
        }
        PsxExe::Parse(blob, *Exe);
      }
      
      void AddVfs(const Binary::Container& blob)
      {
        Require(!Exe);
        if (!Vfs)
        {
          Vfs = MakeRWPtr<PsxVfs>();
        }
        PsxVfs::Parse(blob, *Vfs);
      }
      
      void AddMeta(const XSF::MetaInformation& meta)
      {
        if (!Meta)
        {
          Meta = MakeRWPtr<XSF::MetaInformation>(meta);
        }
        else
        {
          Meta->Merge(meta);
        }
      }
      
      ModuleData::Ptr CaptureResult(uint_t version)
      {
        auto res = MakeRWPtr<ModuleData>();
        res->Version = version;
        res->Exe = std::move(Exe);
        res->Vfs = std::move(Vfs);
        res->Meta = std::move(Meta);
        return res;
      }
    private:
      PsxExe::RWPtr Exe;
      PsxVfs::RWPtr Vfs;
      XSF::MetaInformation::RWPtr Meta;
    };
    
    ModuleData::Ptr MergeDependencies() const
    {
      ModuleDataBuilder builder;
      Require(!!Head.ProgramSection != !!Head.ReservedSection);
      if (Head.ProgramSection)
      {
        MergeExe(Head, builder);
      }
      else
      {
        MergeVfs(Head, builder);
      }
      return builder.CaptureResult(Head.Version);
    }
    
    void MergeExe(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      Require(!!data.ProgramSection);
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim)
      {
        MergeExe(GetDependency(*it), dst);
      }
      dst.AddExe(*data.ProgramSection);
      if (data.Meta)
      {
        dst.AddMeta(*data.Meta);
      }
      if (it != lim)
      {
        for (++it; it != lim; ++it)
        {
          MergeExe(GetDependency(*it), dst);
        }
      }
    }
    
    void MergeVfs(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      Require(!!data.ReservedSection);
      for (const auto& dep : data.Dependencies)
      {
        MergeVfs(GetDependency(dep), dst);
      }
      dst.AddVfs(*data.ReservedSection);
      if (data.Meta)
      {
        dst.AddMeta(*data.Meta);
      }
    }
    
    const XSF::File& GetDependency(const String& name) const
    {
      const auto it = Dependencies.find(name);
      if (it == Dependencies.end() || 0 == it->second.Version)
      {
        Dbg("PSF%1%: unresolved '%2%'", Head.Version, name);
        throw MakeFormattedError(THIS_LINE, "Unresolved dependency '%1%'", name);
      }
      Dbg("PSF%1%: apply '%2%'", Head.Version, name);
      return it->second;
    }
    
    void FillStrings() const
    {
      Strings::Array linear;
      linear.reserve(Dependencies.size());
      for (const auto& dep : Dependencies)
      {
        linear.push_back(dep.first);
      }
      PropertiesHelper(*Properties).SetStrings(linear);
    }
  private:
    const Time::Seconds DefaultDuration;
    mutable Parameters::Container::Ptr Properties;
    mutable XSF::File Head;
    mutable std::map<String, XSF::File> Dependencies;
    
    mutable Holder::Ptr Delegate;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        Dbg("Try to parse PSF/PSF2");
        XSF::File file;
        if (const auto source = XSF::Parse(rawData, file))
        {
          PropertiesHelper props(*properties);
          props.SetSource(*source);

          const XsfView xsf(file);
          if (xsf.IsSingleTrack())
          {
            Dbg("Singlefile PSF/PSF2");
            auto tune = xsf.CreateModuleData();
            return Holder::Create(std::move(tune), std::move(properties), GetDuration(params));
          }
          else if (xsf.IsMultiTrack())
          {
            Dbg("Multifile PSF/PSF2");
            return MakePtr<MultiFileHolder>(std::move(file), std::move(properties), GetDuration(params));
          }
          else
          {
            Dbg("Invalid PSF/PSF2");
          }
        }
      }
      catch (const std::exception&)
      {
        Dbg("Failed to parse PSF");
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
