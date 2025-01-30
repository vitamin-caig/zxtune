/**
 *
 * @file
 *
 * @brief  SNDH chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/sndh.h"
#include "module/players/aym/ay_data_channel.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/sc68_replayers.h"
#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "binary/dump.h"
#include "binary/view.h"
#include "debug/log.h"
#include "formats/multitrack/sndh.h"
#include "math/numeric.h"
#include "module/players/duration.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"
#include "time/oscillator.h"
#include <algorithm>
#include "3rdparty/ht/Core/c68k/c68k.h"

namespace Module::SNDH
{
  const Debug::Stream Dbg("Core::SNDHSupp");

  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;

    Time::Seconds Duration;
    uint_t Frequency = 50;

    uint32_t PlayAddr = 0;
    uint32_t Index = 0;
    String Replayer;
    Binary::Dump RAM;
  };

  // from api68.c
  const uint8_t TRAP_14[] = {
      0x0c, 0x6f, 0x00, 0x26, 0x00, 0x06, 0x67, 0x00, 0x00, 0x22, 0x0c, 0x6f, 0x00, 0x1f, 0x00, 0x06, 0x67, 0x00, 0x00,
      0x28, 0x0c, 0x6f, 0x00, 0x22, 0x00, 0x06, 0x67, 0x00, 0x00, 0x86, 0x0c, 0x6f, 0x00, 0x0e, 0x00, 0x06, 0x67, 0x00,
      0x00, 0xec, 0x4e, 0x73, 0x48, 0xe7, 0xff, 0xfe, 0x20, 0x6f, 0x00, 0x44, 0x4e, 0x90, 0x4c, 0xdf, 0x7f, 0xff, 0x4e,
      0x73, 0x48, 0xe7, 0xff, 0xfe, 0x41, 0xef, 0x00, 0x44, 0x4c, 0x98, 0x00, 0x07, 0x02, 0x40, 0x00, 0x03, 0xd0, 0x40,
      0x16, 0x38, 0xfa, 0x17, 0x02, 0x43, 0x00, 0xf0, 0xe5, 0x4b, 0x36, 0x7b, 0x00, 0x42, 0xd6, 0xc3, 0x76, 0x00, 0x45,
      0xf8, 0xfa, 0x1f, 0xd4, 0xc0, 0x43, 0xf8, 0xfa, 0x19, 0xd2, 0xc0, 0xe2, 0x48, 0x04, 0x00, 0x00, 0x02, 0x6b, 0x1a,
      0x57, 0xc3, 0x18, 0x03, 0x0a, 0x03, 0x00, 0xf0, 0x44, 0x04, 0x48, 0x84, 0xd8, 0x44, 0x43, 0xf1, 0x40, 0xfe, 0xd8,
      0x44, 0x02, 0x41, 0x00, 0x0f, 0xe9, 0x69, 0xc7, 0x11, 0x14, 0x82, 0x26, 0x90, 0x83, 0x11, 0x4c, 0xdf, 0x7f, 0xff,
      0x4e, 0x73, 0x00, 0x34, 0x00, 0x20, 0x00, 0x14, 0x00, 0x10, 0x48, 0xe7, 0x00, 0xc0, 0x43, 0xfa, 0x00, 0x20, 0x70,
      0x00, 0x20, 0x7b, 0x00, 0x3e, 0x41, 0xfb, 0x80, 0x5e, 0x23, 0x88, 0x00, 0x00, 0x58, 0x40, 0xb0, 0x7c, 0x00, 0x24,
      0x66, 0xec, 0x20, 0x09, 0x4c, 0xdf, 0x03, 0x00, 0x4e, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x75, 0xc1, 0x88, 0x41, 0xfa, 0x00, 0x0c, 0x21, 0x48, 0x00, 0x04, 0x58,
      0x48, 0xc1, 0x88, 0x4e, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x03,
  };

  class RAMRegion
  {
    static constexpr const uint32_t BE_XOR = isLE() ? 0 : 1;
    static constexpr const std::size_t MIN_SIZE = 0x80000;  // 512k
  public:
    RAMRegion()
      : Content(MIN_SIZE)
    {}

    void SetRaw(uint32_t addr, Binary::View mem)
    {
      EnsureAccessible(addr + mem.Size() - 1);
      auto* dst = LookupWord(addr);
      std::copy_n(mem.As<be_uint16_t>(), mem.Size() / sizeof(be_uint16_t), dst);
    }

    void SetTrap(uint_t idx, uint32_t addr, Binary::View mem)
    {
      SetDword(0x80 + idx * sizeof(addr), addr);
      SetRaw(addr, mem);
    }

    void SetDword(uint32_t addr, uint32_t value)
    {
      auto* target = LookupWord(addr);
      target[0] = value >> 16;
      target[1] = value & 0xffff;
    }

    std::size_t GetSize() const
    {
      return Content.size();
    }

    uint8_t* LookupByte(uint32_t addr)
    {
      return Content.data() + (WrapAddr(addr) ^ BE_XOR);
    }

    uint16_t* LookupWord(uint32_t addr)
    {
      return static_cast<uint16_t*>(static_cast<void*>(Content.data())) + WrapAddr(addr) / sizeof(uint16_t);
    }

  private:
    void EnsureAccessible(std::size_t addr)
    {
      std::size_t memSize = MIN_SIZE;
      while (memSize < addr)
      {
        memSize *= 2;
      }
      if (memSize > Content.size())
      {
        Dbg("Increase RAM to {}kB", memSize / 1024);
        Content.resize(memSize);
      }
    }

    uint32_t WrapAddr(uint32_t addr) const
    {
      return addr & (Content.size() - 1);
    }

  private:
    Binary::Dump Content;
  };

  const auto CPU_FREQ = 8000000;
  using TimeUnit = Time::BaseUnit<uint64_t, 1ull << 33>;
  using Stamp = Time::Instant<TimeUnit>;
  using Oscillator = Time::Oscillator<TimeUnit>;

  class IODevice
  {
  public:
    using Ptr = std::unique_ptr<IODevice>;
    virtual ~IODevice() = default;

    virtual uint8_t ReadByte(uint32_t addr) = 0;
    virtual uint16_t ReadWord(uint32_t addr) = 0;
    virtual void WriteByte(const Oscillator& timeStamp, uint32_t addr, uint8_t data) = 0;
    virtual void WriteWord(const Oscillator& timeStamp, uint32_t addr, uint16_t data) = 0;
  };

  static class DummyDevice : public IODevice
  {
  public:
    uint8_t ReadByte(uint32_t /*addr*/) override
    {
      return 0;
    }

    uint16_t ReadWord(uint32_t /*addr*/) override
    {
      return 0;
    }

    void WriteByte(const Oscillator& /*timeStamp*/, uint32_t /*addr*/, uint8_t /*data*/) override {}
    void WriteWord(const Oscillator& /*timeStamp*/, uint32_t /*addr*/, uint16_t /*data*/) override {}
  } DUMMY;

  class IOBus
  {
  public:
    IOBus(const Oscillator& timeStamp, RAMRegion& ram)
      : TimeStamp(timeStamp)
      , Ram(ram)
    {
      Mapping.fill(&DUMMY);
    }

    uint8_t ReadByte(uint32_t addr)
    {
      return IsIOAddr(addr) ? GetDevice(addr)->ReadByte(addr) : *Ram.LookupByte(addr);
    }

    uint16_t ReadWord(uint32_t addr)
    {
      return IsIOAddr(addr) ? GetDevice(addr)->ReadWord(addr) : *Ram.LookupWord(addr);
    }

    void WriteByte(uint32_t addr, uint8_t data)
    {
      if (IsIOAddr(addr))
      {
        GetDevice(addr)->WriteByte(TimeStamp, addr, data);
      }
      else
      {
        *Ram.LookupByte(addr) = data;
      }
    }

    void WriteWord(uint32_t addr, uint16_t data)
    {
      if (IsIOAddr(addr))
      {
        GetDevice(addr)->WriteWord(TimeStamp, addr, data);
      }
      else
      {
        *Ram.LookupWord(addr) = data;
      }
    }

    void AddDevice(uint32_t rangeStart, uint32_t rangeEnd, IODevice::Ptr dev)
    {
      for (uint_t idx = ToLookup(rangeStart), lim = ToLookup(rangeEnd); idx < lim; ++idx)
      {
        Mapping[idx] = dev.get();
      }
      Devices.emplace_back(std::move(dev));
    }

  private:
    static bool IsIOAddr(uint32_t addr)
    {
      return addr & 0x800000;
    }

    static uint_t ToLookup(uint32_t addr)
    {
      return (addr >> 8) & 0xff;
    }

    IODevice* GetDevice(uint32_t addr) const
    {
      return Mapping[ToLookup(addr)];
    }

  private:
    const Oscillator& TimeStamp;
    RAMRegion& Ram;
    std::array<IODevice*, 256> Mapping;
    std::vector<IODevice::Ptr> Devices;
  };

  class CPUDevice
  {
  public:
    CPUDevice(IOBus& io)
    {
      ::C68k_Init(&Context, nullptr);
      ::C68k_Set_Callback_Param(&Context, &io);
      ::C68k_Set_ReadB(&Context, &ReadByteTrampoline);
      ::C68k_Set_ReadW(&Context, &ReadWordTrampoline);
      ::C68k_Set_WriteB(&Context, &WriteByteTrampoline);
      ::C68k_Set_WriteW(&Context, &WriteWordTrampoline);
    }

    void SetFetch(const void* data, std::size_t size)
    {
      ::C68k_Set_Fetch(&Context, 0, size - 1, reinterpret_cast<pointer>(data));
    }

    void SetRegD(uint_t idx, uint32_t value)
    {
      ::C68k_Set_DReg(&Context, idx, value);
    }

    void SetRegA(uint_t idx, uint32_t value)
    {
      ::C68k_Set_AReg(&Context, idx, value);
    }

    uint32_t GetRegA(uint_t idx)
    {
      return ::C68k_Get_AReg(&Context, idx);
    }

    void SetRegPC(uint32_t value)
    {
      ::C68k_Set_PC(&Context, value);
    }

    void SetRegSR(uint32_t value)
    {
      ::C68k_Set_SR(&Context, value);
    }

    uint_t ExecuteStep(uint_t ticksToDo)
    {
      const auto done = ::C68k_Exec(&Context, ticksToDo);
      Require(done >= 0);
      return done;
    }

    void Interrupt(uint_t level)
    {
      ::C68k_Set_IRQ(&Context, level);
    }

  private:
    static u32 FASTCALL ReadByteTrampoline(void* param, const u32 address)
    {
      return static_cast<IOBus*>(param)->ReadByte(address);
    }

    static u32 FASTCALL ReadWordTrampoline(void* param, const u32 address)
    {
      return static_cast<IOBus*>(param)->ReadWord(address);
    }

    static void FASTCALL WriteByteTrampoline(void* param, const u32 address, u32 data)
    {
      return static_cast<IOBus*>(param)->WriteByte(address, data);
    }

    static void FASTCALL WriteWordTrampoline(void* param, const u32 address, u32 data)
    {
      return static_cast<IOBus*>(param)->WriteWord(address, data);
    }

  private:
    c68k_struc Context;
  };

  class AyIO : public IODevice
  {
  public:
    explicit AyIO(AyDataChannel& ay)
      : Ay(ay)
    {}

    uint8_t ReadByte(uint32_t addr) override
    {
      return (addr & 3) ? 0 : Ay.GetValue();
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      return ReadByte(addr) << 8;
    }

    void WriteByte(const Oscillator& timeStamp, uint32_t addr, uint8_t data) override
    {
      if (addr & 2)
      {
        Ay.SetValue(timeStamp.GetCurrentTime().CastTo<Devices::AYM::TimeUnit>(), data);
      }
      else
      {
        Ay.SelectRegister(data);
      }
    }

    void WriteWord(const Oscillator& timeStamp, uint32_t addr, uint16_t data) override
    {
      WriteByte(timeStamp, addr, data >> 8);
    }

  private:
    AyDataChannel& Ay;
  };

  class Computer
  {
  public:
    using Ptr = std::unique_ptr<Computer>;

    Computer()
      : Bus(Clock, Ram)
      , Cpu(Bus)
    {}

    void Add(AyDataChannel& ay)
    {
      Bus.AddDevice(0xffff8800, 0xffff88ff, MakePtr<AyIO>(ay));
    }

    void Start(const ModuleData& data)
    {
      Clock.SetFrequency(CPU_FREQ);
      Ram.SetDword(0, 0x4e730000);
      Ram.SetTrap(14, 0x600, TRAP_14);
      const auto target = PutReplayer(data.PlayAddr, data.Replayer);
      Ram.SetRaw(target, data.RAM);
      Cpu.SetFetch(Ram.LookupWord(0), Ram.GetSize());
      Initialize(data.PlayAddr, data.Index, target, data.RAM.size());
    }

    void Execute(Time::Microseconds duration)
    {
      Cpu.Interrupt(6);
      const auto targetTime = Clock.GetCurrentTime() + duration;
      const auto targetTick = Clock.GetTickAtTime(targetTime);
      while (Clock.GetCurrentTick() < targetTick)
      {
        const auto done = Cpu.ExecuteStep(targetTick - Clock.GetCurrentTick());
        Clock.AdvanceTick(done);
      }
    }

    auto GetCurrentTime() const
    {
      return Clock.GetCurrentTime();
    }

  private:
    uint32_t PutReplayer(uint32_t addr, StringView name)
    {
      if (const auto content = SC68::GetReplayer(name))
      {
        Ram.SetRaw(addr, content);
        addr += Math::Align<uint32_t>(content.Size(), sizeof(uint16_t));
      }
      return addr;
    }

    void Initialize(uint32_t play, uint32_t idx, uint32_t moduleData, std::size_t moduleSize)
    {
      const auto sp = Ram.GetSize() - 16;
      Cpu.SetRegD(0, idx + 1);
      Cpu.SetRegD(2, moduleSize);
      Cpu.SetRegA(0, moduleData);
      Cpu.SetRegA(7, sp - sizeof(uint32_t));
      Cpu.SetRegPC(play);
      Cpu.SetRegSR(0x2300);
      while (Cpu.GetRegA(7) < sp)
      {
        Cpu.ExecuteStep(1);
      }

      Cpu.SetRegPC(play + 2 * sizeof(uint32_t));
      Cpu.SetRegSR(0x2300);
    }

  private:
    Oscillator Clock;
    RAMRegion Ram;
    IOBus Bus;
    CPUDevice Cpu;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr tune, Devices::AYM::Chip::Ptr ay)
      : Tune(std::move(tune))
      , State(MakePtr<TimedState>(Tune->Duration))
      , Ay(std::move(ay))
      , Engine(MakeEngine())
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      Engine->Execute(avail);
      return Ay.RenderFrame(Engine->GetCurrentTime().CastTo<Devices::AYM::TimeUnit>());
    }

    void Reset() override
    {
      State->Reset();
      Ay.Reset();
      Engine = MakeEngine();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine = MakeEngine();
      }
      if (const auto toSkip = State->Seek(request))
      {
        // Engine->Skip(GetSamples(toSkip));
      }
    }

  private:
    Computer::Ptr MakeEngine()
    {
      auto res = MakePtr<Computer>();
      res->Add(Ay);
      res->Start(*Tune);
      return res;
    }

  private:
    const ModuleData::Ptr Tune;
    const TimedState::Ptr State;
    AyDataChannel Ay;
    Computer::Ptr Engine;
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
      return CreateTimedInfo(Tune->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto aym = AYM::CreateChip(samplerate, params);
      return MakePtr<Renderer>(Tune, std::move(aym));
    }

  private:
    const ModuleData::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Multitrack::SNDH::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Module(MakeRWPtr<ModuleData>())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetYear(StringView year) override
    {
      Properties.SetDate(year);
    }

    void SetUnknownTag(StringView /*tag*/, StringView /*value*/) override {}

    void SetTimer(char /*type*/, uint_t freq) override
    {
      Module->Frequency = freq;
    }

    void SetSubtunes(std::vector<StringView> names) override
    {
      Names = std::move(names);
    }

    void SetDurations(std::vector<Time::Seconds> durations) override
    {
      Durations = std::move(durations);
    }

    ModuleData::RWPtr CaptureResult(uint_t index)
    {
      Module->Index = index;
      Module->PlayAddr = 0x8000;
      if (index < Durations.size())
      {
        Module->Duration = Durations[index];
      }
      if (index < Names.size())
      {
        Properties.SetTitle(Names[index]);
      }
      return {std::move(Module)};
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    ModuleData::RWPtr Module;
    std::vector<StringView> Names;
    std::vector<Time::Seconds> Durations;
  };

  Binary::Dump Copy(Binary::View data)
  {
    return {data.As<uint8_t>(), data.As<uint8_t>() + data.Size()};
  }

  class Factory : public MultitrackFactory
  {
  public:
    Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Multitrack::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      DataBuilder builder(props);
      Require(Formats::Multitrack::SNDH::Parse(data, builder) != nullptr);
      props.SetPlatform(Platforms::ATARI);
      auto tune = builder.CaptureResult(data.StartTrackIndex());
      if (!tune->Duration)
      {
        tune->Duration = GetDefaultDuration(params);
      }
      tune->Replayer = "sndh-ice";
      tune->RAM = Copy(data);
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }
  };

  MultitrackFactory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::SNDH
