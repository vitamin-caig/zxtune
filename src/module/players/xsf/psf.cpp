/**
 *
 * @file
 *
 * @brief  PSF chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/xsf/psf.h"

#include "module/players/platforms.h"
#include "module/players/streaming.h"
#include "module/players/xsf/psf_bios.h"
#include "module/players/xsf/psf_exe.h"
#include "module/players/xsf/psf_vfs.h"
#include "module/players/xsf/xsf.h"

#include "binary/compression/zlib_container.h"
#include "debug/log.h"
#include "module/attributes.h"
#include "sound/resampler.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include "3rdparty/he/Core/bios.h"
#include "3rdparty/he/Core/iop.h"
#include "3rdparty/he/Core/psx.h"
#include "3rdparty/he/Core/r3000.h"
#include "3rdparty/he/Core/spu.h"

#include <utility>

namespace Module::PSF
{
  const Debug::Stream Dbg("Module::PSF");

  class VfsIO
  {
  public:
    VfsIO() = default;
    explicit VfsIO(PsxVfs::Ptr vfs)
      : Vfs(std::move(vfs))
    {}

    sint32 Read(const char* path, sint32 offset, char* buffer, sint32 length)
    {
      if (PreloadFile(path))
      {
        if (length == 0)
        {
          const auto result = GetSize();
          Dbg("Size()={}", result);
          return result;
        }
        else
        {
          const auto result = Read(offset, buffer, length);
          Dbg("Read({}@{})={}", length, offset, result);
          return result;
        }
      }
      return -1;
    }

  private:
    bool PreloadFile(StringView path) const
    {
      if (CachedName == path)
      {
        return true;
      }
      if (auto data = Vfs->Find(path))
      {
        Dbg("Open '{}'", path);
        CachedName = path;
        CachedData = std::move(data);
        return true;
      }
      else
      {
        Dbg("Not found '{}'", path);
        return false;
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
        return 60;  // NTSC by default
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
    static std::unique_ptr<uint8_t[]> CreatePSX(int version)
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

  const SpuTrait SPU1 = {0x1f801c00, 0x4, 0xc};  // mirrored at {0x1f900000, 0x4, 0xa} for PS2
  const SpuTrait SPU2 = {0x1f900400, 0x4, 0xa};

  class PSXEngine
  {
  public:
    using Ptr = std::shared_ptr<PSXEngine>;

    explicit PSXEngine(const ModuleData& data)
    {
      Initialize(data);
    }

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
      for (uint32_t doneSamples = 0; doneSamples < samples;)
      {
        uint32_t toRender = samples - doneSamples;
        const auto res =
            ::psx_execute(Emu.get(), 0x7fffffff, safe_ptr_cast<short int*>(&result[doneSamples]), &toRender, 0);
        Require(res >= 0);
        Require(toRender != 0);
        doneSamples += toRender;
      }
      return result;
    }

    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples;)
      {
        uint32_t toSkip = samples - skippedSamples;
        const auto res = ::psx_execute(Emu.get(), 0x7fffffff, nullptr, &toSkip, 0);
        Require(res >= 0);
        Require(toSkip != 0);
        skippedSamples += toSkip;
      }
    }

  private:
    void SetupExe(const PsxExe& exe)
    {
      SetRAM(exe.RAM);
      SetRegisters(exe.PC, exe.SP);
    }

    void SetRAM(const MemoryRegion& mem)
    {
      auto* const iop = ::psx_get_iop_state(Emu.get());
      ::iop_upload_to_ram(iop, mem.Start, mem.Data.data(), mem.Data.size());
    }

    void SetRegisters(uint32_t pc, uint32_t sp)
    {
      auto* const iop = ::psx_get_iop_state(Emu.get());
      auto* const cpu = ::iop_get_r3000_state(iop);
      ::r3000_setreg(cpu, R3000_REG_PC, pc);
      ::r3000_setreg(cpu, R3000_REG_GEN + 29, sp);
    }

    void SetupIo(PsxVfs::Ptr vfs)
    {
      Io = VfsIO(std::move(vfs));
      ::psx_set_readfile(Emu.get(), &ReadCallback, &Io);
    }

    static sint32 ReadCallback(void* context, const char* path, sint32 offset, char* buffer, sint32 length)
    {
      auto* const io = static_cast<VfsIO*>(context);
      return io->Read(path, offset, buffer, length);
    }

  private:
    uint_t SoundFrequency = 0;
    std::vector<SpuTrait> Spus;
    std::unique_ptr<uint8_t[]> Emu;
    VfsIO Io;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, uint_t samplerate)
      : Data(std::move(data))
      , State(MakePtr<TimedState>(Data->Meta->Duration))
      , Engine(MakePtr<PSXEngine>(*Data))
      , Target(Sound::CreateResampler(Engine->GetSoundFrequency(), samplerate))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Target->Apply(Engine->Render(GetSamples(avail)));
    }

    void Reset() override
    {
      State->Reset();
      Engine->Initialize(*Data);
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine->Initialize(*Data);
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine->Skip(GetSamples(toSkip));
      }
    }

  private:
    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine->GetSoundFrequency() / period.PER_SECOND;
    }

  private:
    const ModuleData::Ptr Data;
    const TimedState::Ptr State;
    const PSXEngine::Ptr Engine;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Meta->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Tune, samplerate);
    }

    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      properties->SetValue(ATTR_PLATFORM, tune->Version == 1 ? Platforms::PLAYSTATION : Platforms::PLAYSTATION_2);
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }

  private:
    const ModuleData::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class ModuleDataBuilder
  {
  public:
    void AddExe(Binary::View packedSection)
    {
      Require(!Vfs);
      if (!Exe)
      {
        Exe = MakeRWPtr<PsxExe>();
      }
      const auto unpackedSection = Binary::Compression::Zlib::Decompress(packedSection);
      PsxExe::Parse(*unpackedSection, *Exe);
    }

    void AddVfs(const Binary::Container& reservedSection)
    {
      Require(!Exe);
      if (!Vfs)
      {
        Vfs = MakeRWPtr<PsxVfs>();
      }
      PsxVfs::Parse(reservedSection, *Vfs);
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

  class Factory : public XSF::Factory
  {
  public:
    Holder::Ptr CreateSinglefileModule(const XSF::File& file, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      if (file.PackedProgramSection)
      {
        builder.AddExe(*file.PackedProgramSection);
      }
      if (file.ReservedSection)
      {
        builder.AddVfs(*file.ReservedSection);
      }
      if (file.Meta)
      {
        builder.AddMeta(*file.Meta);
      }
      return Holder::Create(builder.CaptureResult(file.Version), std::move(properties));
    }

    Holder::Ptr CreateMultifileModule(const XSF::File& file, const XSF::FilesMap& additionalFiles,
                                      Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      if (file.PackedProgramSection)
      {
        MergeExe(file, additionalFiles, builder);
      }
      if (file.ReservedSection)
      {
        MergeVfs(file, additionalFiles, builder);
      }
      MergeMeta(file, additionalFiles, builder);
      return Holder::Create(builder.CaptureResult(file.Version), std::move(properties));
    }

  private:
    /* https://bitbucket.org/zxtune/zxtune/wiki/MiniPSF

    The proper way to load a minipsf is as follows:
    - Load the executable data from the minipsf - this becomes the current executable.
    - Check for the presence of a "_lib" tag. If present:
      - RECURSIVELY load the executable data from the given library file. (Make sure to limit recursion to avoid
    crashing - I usually limit it to 10 levels)
      - Make the _lib executable the current one.
      - If applicable, we will use the initial program counter/stack pointer from the _lib executable.
      - Superimpose the originally loaded minipsf executable on top of the current executable. If applicable, use the
    start address and size to determine where to .
    - Check for the presence of "_libN" tags for N=2 and up (use "_lib%d")
      - RECURSIVELY load and superimpose all these EXEs on top of the current EXE. Do not modify the current program
    counter or stack pointer.
      - Start at N=2. Stop at the first tag name that doesn't exist.
    - (done)
    */
    static const uint_t MAX_LEVEL = 10;

    static void MergeExe(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                         uint_t level = 1)
    {
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim && level < MAX_LEVEL)
      {
        MergeExe(additionalFiles.at(*it), additionalFiles, dst, level + 1);
      }
      dst.AddExe(*data.PackedProgramSection);
      if (it != lim && level < MAX_LEVEL)
      {
        for (++it; it != lim; ++it)
        {
          MergeExe(additionalFiles.at(*it), additionalFiles, dst, level + 1);
        }
      }
    }

    /* https://bitbucket.org/zxtune/zxtune/wiki/MiniPSF2

    The proper way to load a MiniPSF2 is as follows:
    - First, recursively load the virtual filesystems from each PSF2 file named by a library tag.
      - The first tag is "_lib"
      - The remaining tags are "_libN" for N>=2 (use "_lib%d")
      - Stop at the first tag name that doesn't exist.
    - Then, load the virtual filesystem from the current PSF2 file.

    If there are conflicting or redundant filenames, they should be overwritten in memory in the order in which the
    filesystem data was parsed. Later takes priority.
    */
    static void MergeVfs(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                         uint_t level = 1)
    {
      if (level < MAX_LEVEL)
      {
        for (const auto& dep : data.Dependencies)
        {
          MergeVfs(additionalFiles.at(dep), additionalFiles, dst, level + 1);
        }
      }
      dst.AddVfs(*data.ReservedSection);
    }

    static void MergeMeta(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                          uint_t level = 1)
    {
      if (level < MAX_LEVEL)
      {
        for (const auto& dep : data.Dependencies)
        {
          MergeMeta(additionalFiles.at(dep), additionalFiles, dst, level + 1);
        }
      }
      if (data.Meta)
      {
        dst.AddMeta(*data.Meta);
      }
    }
  };

  XSF::Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::PSF
