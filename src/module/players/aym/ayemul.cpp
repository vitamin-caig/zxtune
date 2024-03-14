/**
 *
 * @file
 *
 * @brief  AY EMUL chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/ayemul.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <core/core_parameters.h>
#include <debug/log.h>
#include <devices/beeper.h>
#include <devices/z80.h>
#include <formats/chiptune/emulation/ay.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
// std includes
#include <algorithm>
#include <utility>

namespace Module::AYEMUL
{
  const Debug::Stream Dbg("Core::AYSupp");

  class AyDataChannel
  {
  public:
    explicit AyDataChannel(Devices::AYM::Chip::Ptr chip)
      : Chip(std::move(chip))
    {}

    void Reset()
    {
      Chip->Reset();
      Register = 0;
      Chunks.clear();
      State = Devices::AYM::DataChunk();
      Blocked = false;
    }

    void SetBlocked(bool block)
    {
      Blocked = block;
    }

    bool SelectRegister(uint_t reg)
    {
      Register = reg;
      return IsRegisterSelected();
    }

    bool SetValue(const Devices::AYM::Stamp& timeStamp, uint8_t val)
    {
      if (IsRegisterSelected())
      {
        const auto idx = static_cast<Devices::AYM::Registers::Index>(Register);
        if (Devices::AYM::DataChunk* chunk = GetChunk(timeStamp))
        {
          chunk->Data[idx] = val;
        }
        State.Data[idx] = val;
        return true;
      }
      return false;
    }

    uint8_t GetValue() const
    {
      if (IsRegisterSelected())
      {
        return State.Data[static_cast<Devices::AYM::Registers::Index>(Register)];
      }
      else
      {
        return 0xff;
      }
    }

    Sound::Chunk RenderFrame(const Devices::AYM::Stamp& till)
    {
      AllocateChunk(till);
      Chip->RenderData(Chunks);
      Chunks.clear();
      return Chip->RenderTill(till);
    }

  private:
    bool IsRegisterSelected() const
    {
      return Register < Devices::AYM::Registers::TOTAL;
    }

    Devices::AYM::DataChunk* GetChunk(const Devices::AYM::Stamp& timeStamp)
    {
      return Blocked ? nullptr : &AllocateChunk(timeStamp);
    }

    Devices::AYM::DataChunk& AllocateChunk(const Devices::AYM::Stamp& timeStamp)
    {
      Chunks.resize(Chunks.size() + 1);
      Devices::AYM::DataChunk& res = Chunks.back();
      res.TimeStamp = timeStamp;
      return res;
    }

  private:
    const Devices::AYM::Chip::Ptr Chip;
    uint_t Register = 0;
    std::vector<Devices::AYM::DataChunk> Chunks;
    Devices::AYM::DataChunk State;
    bool Blocked = false;
  };

  class BeeperDataChannel
  {
  public:
    explicit BeeperDataChannel(Devices::Beeper::Chip::Ptr chip)
      : Chip(std::move(chip))
    {}

    void Reset()
    {
      Chip->Reset();
      Chunks.clear();
      State = false;
      Blocked = false;
    }

    void SetBlocked(bool block)
    {
      Blocked = block;
    }

    void SetLevel(const Devices::Beeper::Stamp& timeStamp, bool val)
    {
      if (val != State)
      {
        State = val;
        if (!Blocked)
        {
          AllocateChunk(timeStamp);
        }
      }
    }

    Sound::Chunk RenderFrame(const Devices::AYM::Stamp& till)
    {
      if (Chunks.empty())
      {
        Chip->RenderTill(till);
        return {};
      }
      Chip->RenderData(Chunks);
      Chunks.clear();
      return Chip->RenderTill(till);
    }

  private:
    void AllocateChunk(Devices::Beeper::Stamp timeStamp)
    {
      Chunks.emplace_back(timeStamp, State);
    }

  private:
    const Devices::Beeper::Chip::Ptr Chip;
    std::vector<Devices::Beeper::DataChunk> Chunks;
    bool State = false;
    bool Blocked = false;
  };

  class DataChannel
  {
  public:
    using Ptr = std::shared_ptr<DataChannel>;

    DataChannel(Devices::AYM::Chip::Ptr ay, Devices::Beeper::Chip::Ptr beep)
      : Ay(std::move(ay))
      , Beeper(std::move(beep))
    {}

    void Reset()
    {
      Ay.Reset();
      Beeper.Reset();
    }

    void SetBlocked(bool block)
    {
      Ay.SetBlocked(block);
      Beeper.SetBlocked(block);
    }

    bool SelectAyRegister(uint_t reg)
    {
      return Ay.SelectRegister(reg);
    }

    bool SetAyValue(const Devices::Z80::Stamp& timeStamp, uint8_t val)
    {
      return Ay.SetValue(timeStamp.CastTo<Devices::AYM::TimeUnit>(), val);
    }

    uint8_t GetAyValue() const
    {
      return Ay.GetValue();
    }

    void SetBeeperValue(const Devices::Z80::Stamp& timeStamp, bool val)
    {
      Beeper.SetLevel(timeStamp.CastTo<Devices::Beeper::TimeUnit>(), val);
    }

    Sound::Chunk RenderFrameTill(Time::AtMicrosecond till)
    {
      auto aySound = Ay.RenderFrame(till.CastTo<Devices::AYM::TimeUnit>());
      auto beepSound = Beeper.RenderFrame(till.CastTo<Devices::Beeper::TimeUnit>());
      if (!beepSound.empty())
      {
        if (beepSound.size() > aySound.size())
        {
          beepSound.swap(aySound);
        }
        // ay is longer
        std::transform(beepSound.begin(), beepSound.end(), aySound.begin(), aySound.begin(), &MixSamples);
      }
      return aySound;
    }

  private:
    static Sound::Sample MixSamples(Sound::Sample lh, Sound::Sample rh)
    {
      return {(lh.Left() + rh.Left()) / 2, (lh.Right() + rh.Right()) / 2};
    }

  private:
    AyDataChannel Ay;
    BeeperDataChannel Beeper;
  };

  class ZXAYPort
  {
  public:
    explicit ZXAYPort(DataChannel::Ptr chan)
      : Channel(std::move(chan))
    {}

    void Reset()
    {
      Channel->SelectAyRegister(0);
    }

    uint8_t Read(uint16_t port)
    {
      return IsSelRegPort(port) ? Channel->GetAyValue() : 0xff;
    }

    void Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (IsSelRegPort(port))
      {
        Channel->SelectAyRegister(data);
      }
      else if (IsSetValPort(port))
      {
        Channel->SetAyValue(timeStamp.GetCurrentTime(), data);
      }
      else if (IsBeeperPort(port))
      {
        Channel->SetBeeperValue(timeStamp.GetCurrentTime(), 0 != (data & 16));
      }
    }

  private:
    static const uint16_t ZX_AY_PORT_MASK = 0xc002;

    static bool IsSelRegPort(uint16_t port)
    {
      const uint16_t SEL_REG_PORT = 0xfffd;
      return (ZX_AY_PORT_MASK & SEL_REG_PORT) == (ZX_AY_PORT_MASK & port);
    }

    static bool IsSetValPort(uint16_t port)
    {
      const uint16_t SET_VAL_PORT = 0xbffd;
      return (ZX_AY_PORT_MASK & SET_VAL_PORT) == (ZX_AY_PORT_MASK & port);
    }

    static bool IsBeeperPort(uint16_t port)
    {
      return 0 == (port & 0x0001);
    }

  private:
    const DataChannel::Ptr Channel;
  };

  class CPCAYPort
  {
  public:
    explicit CPCAYPort(DataChannel::Ptr channel)
      : Channel(std::move(channel))
    {}

    void Reset()
    {
      Channel->SelectAyRegister(0);
      Data = 0;
      Selector = 0;
    }

    static uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (IsDataPort(port))
      {
        Data = data;
        // data means nothing
      }
      else if (IsControlPort(port))
      {
        data &= 0xc0;
        bool res = false;
        if (!Selector)
        {
          Selector = data;
        }
        else if (!data)
        {
          switch (Selector & 0xc0)
          {
          case 0xc0:
            res = Channel->SelectAyRegister(Data);
            break;
          case 0x80:
            res = Channel->SetAyValue(timeStamp.GetCurrentTime(), Data);
            break;
          }
          Selector = 0;
        }
        return res;
      }
      return false;
    }

  private:
    static bool IsDataPort(uint16_t port)
    {
      return 0xf400 == (port & 0xff00);
    }

    static bool IsControlPort(uint16_t port)
    {
      return 0xf600 == (port & 0xff00);
    }

  private:
    const DataChannel::Ptr Channel;
    uint8_t Data = '\0';
    uint_t Selector = 0;
  };

  class PortsPlexer : public Devices::Z80::ChipIO
  {
  public:
    explicit PortsPlexer(DataChannel::Ptr channel)
      : Channel(channel)
      , ZX(channel)
      , CPC(std::move(channel))
    {}
    using Ptr = std::shared_ptr<PortsPlexer>;

    static Ptr Create(DataChannel::Ptr ayData)
    {
      return MakePtr<PortsPlexer>(std::move(ayData));
    }

    void Reset()
    {
      ZX.Reset();
      CPC.Reset();
    }

    void SetBlocked(bool blocked)
    {
      Channel->SetBlocked(blocked);
    }

    uint8_t Read(uint16_t port) override
    {
      if (Current)
      {
        return Current->Read(port);
      }
      else
      {
        return ZX.Read(port);
      }
    }

    void Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data) override
    {
      if (Current)
      {
        Current->Write(timeStamp, port, data);
      }
      // check CPC first
      else if (CPC.Write(timeStamp, port, data))
      {
        Dbg("Detected CPC port mapping on write to port #{:04x}", port);
        Current = &CPC;
      }
      else
      {
        // ZX is fallback that will never become current :(
        ZX.Write(timeStamp, port, data);
      }
    }

  private:
    const DataChannel::Ptr Channel;
    ZXAYPort ZX;
    CPCAYPort CPC;
    CPCAYPort* Current = nullptr;
  };

  class CPUParameters : public Devices::Z80::ChipParameters
  {
  public:
    explicit CPUParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t IntTicks() const override
    {
      using namespace Parameters::ZXTune::Core::Z80;
      return Parameters::GetInteger<uint_t>(*Params, INT_TICKS, INT_TICKS_DEFAULT);
    }

    uint64_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::Z80;
      return Parameters::GetInteger<uint64_t>(*Params, CLOCKRATE, CLOCKRATE_DEFAULT);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class ModuleData
  {
  public:
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData() = default;

    Devices::Z80::Chip::Ptr CreateCPU(Devices::Z80::ChipParameters::Ptr params, Devices::Z80::ChipIO::Ptr ports) const
    {
      auto result = Devices::Z80::CreateChip(std::move(params), *Memory, std::move(ports));
      Devices::Z80::Registers regs;
      regs.Mask = ~0;
      std::fill(regs.Data.begin(), regs.Data.end(), Registers);
      regs.Data[Devices::Z80::Registers::REG_SP] = StackPointer;
      regs.Data[Devices::Z80::Registers::REG_IR] = Registers & 0xff;
      regs.Data[Devices::Z80::Registers::REG_PC] = 0;
      result->SetRegisters(regs);
      return result;
    }

    uint_t Frames = 0;
    Time::Microseconds FrameDuration;
    uint16_t Registers = 0;
    uint16_t StackPointer = 0;
    Binary::Data::Ptr Memory;
  };

  class Computer
  {
  public:
    using Ptr = std::shared_ptr<Computer>;

    Computer(ModuleData::Ptr data, Devices::Z80::ChipParameters::Ptr params, PortsPlexer::Ptr cpuPorts)
      : Data(std::move(data))
      , Params(std::move(params))
      , CPUPorts(std::move(cpuPorts))
      , CPU(Data->CreateCPU(Params, CPUPorts))
    {}

    void Reset()
    {
      CPU = Data->CreateCPU(Params, CPUPorts);
      CPUPorts->Reset();
    }

    void ExecuteFrameTill(Time::AtMicrosecond till)
    {
      CPU->Interrupt();
      CPU->Execute(till.CastTo<Devices::Z80::TimeUnit>());
    }

    void SkipFrames(Time::AtMicrosecond from, uint_t count, Time::Microseconds frameStep)
    {
      const auto curTime = CPU->GetTime();
      CPUPorts->SetBlocked(true);
      auto pos = from;
      for (uint_t frame = 0; frame < count; ++frame)
      {
        pos += frameStep;
        ExecuteFrameTill(pos);
      }
      CPUPorts->SetBlocked(false);
      CPU->SetTime(curTime);
    }

  private:
    const ModuleData::Ptr Data;
    const Devices::Z80::ChipParameters::Ptr Params;
    const PortsPlexer::Ptr CPUPorts;
    Devices::Z80::Chip::Ptr CPU;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const ModuleData& data, Computer::Ptr comp, DataChannel::Ptr device)
      : State(MakePtr<TimedState>((data.FrameDuration * data.Frames).CastTo<Time::Millisecond>()))
      , Comp(std::move(comp))
      , Device(std::move(device))
      , FrameDuration(data.FrameDuration)
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      State->ConsumeUpTo(FrameDuration);
      DeviceTime += FrameDuration;
      Comp->ExecuteFrameTill(DeviceTime);
      return Device->RenderFrameTill(DeviceTime);
    }

    void Reset() override
    {
      State->Reset();
      Comp->Reset();
      Device->Reset();
      DeviceTime = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      auto current = State->At();
      if (request < current)
      {
        current = {};
        Comp->Reset();
        Device->Reset();
        DeviceTime = {};
      }
      const auto delta = State->Seek(request);
      if (const auto frames = delta.Divide<uint_t>(FrameDuration))
      {
        // correct logical position
        State->Seek(current + (FrameDuration * frames).CastTo<Time::Millisecond>());
        Comp->SkipFrames(DeviceTime, frames, FrameDuration);
      }
    }

  private:
    const TimedState::Ptr State;
    const Computer::Ptr Comp;
    const DataChannel::Ptr Device;
    const Time::Microseconds FrameDuration;
    // Monotonic time, does not change on fast-forward
    Time::AtMicrosecond DeviceTime;
  };

  class DataBuilder : public Formats::Chiptune::AY::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
      , Delegate(Formats::Chiptune::AY::CreateMemoryDumpBuilder())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetDuration(uint_t duration, uint_t fadeout) override
    {
      Data->Frames = duration;
      if (fadeout)
      {
        Properties.SetFadeout((AYM::BASE_FRAME_DURATION * fadeout).CastTo<Time::Millisecond>());
      }
    }

    void SetRegisters(uint16_t reg, uint16_t sp) override
    {
      Data->Registers = reg;
      Data->StackPointer = sp;
    }

    void SetRoutines(uint16_t init, uint16_t play) override
    {
      Delegate->SetRoutines(init, play);
    }

    void AddBlock(uint16_t addr, Binary::View block) override
    {
      Delegate->AddBlock(addr, block);
    }

    void FillTimingInfo(const Parameters::Accessor& params)
    {
      Data->FrameDuration = AYM::BASE_FRAME_DURATION;
      if (!Data->Frames)
      {
        Data->Frames = GetDefaultDuration(params).Divide<uint_t>(Data->FrameDuration);
      }
    }

    ModuleData::Ptr GetResult() const
    {
      Data->Memory = Delegate->Result();
      return Data;
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    const ModuleData::RWPtr Data;
    const Formats::Chiptune::AY::BlobBuilder::Ptr Delegate;
  };

  class StubBeeper : public Devices::Beeper::Chip
  {
  public:
    void RenderData(const std::vector<Devices::Beeper::DataChunk>& /*src*/) override {}

    void Reset() override {}

    Sound::Chunk RenderTill(Devices::Beeper::Stamp /*till*/) override
    {
      return {};
    }
  };

  class BeeperParams : public Devices::Beeper::ChipParameters
  {
  public:
    BeeperParams(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint64_t ClockFreq() const override
    {
      const uint_t MIN_Z80_TICKS_PER_OUTPUT = 10;
      using namespace Parameters::ZXTune::Core::Z80;
      const auto cpuClock = Parameters::GetInteger<uint64_t>(*Params, CLOCKRATE, CLOCKRATE_DEFAULT);
      return cpuClock / MIN_Z80_TICKS_PER_OUTPUT;
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::Beeper::Chip::Ptr CreateBeeper(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    auto beeperParams = MakePtr<BeeperParams>(samplerate, std::move(params));
    return Devices::Beeper::CreateChip(std::move(beeperParams));
  }

  class Holder : public AYM::Holder
  {
  public:
    Holder(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo((Data->FrameDuration * Data->Frames).CastTo<Time::Millisecond>());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto aym = AYM::CreateChip(samplerate, params);
      auto beeper = CreateBeeper(samplerate, params);
      return CreateRenderer(std::move(params), std::move(aym), std::move(beeper));
    }

    AYM::Chiptune::Ptr GetChiptune() const override
    {
      return {};
    }

    void Dump(Devices::AYM::Device&) const override
    {
      Require(!"Not implemented");
    }

  private:
    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Chip::Ptr ay,
                                 Devices::Beeper::Chip::Ptr beep) const
    {
      auto cpuParams = MakePtr<CPUParameters>(std::move(params));
      auto channel = MakePtr<DataChannel>(std::move(ay), std::move(beep));
      auto cpuPorts = PortsPlexer::Create(channel);
      auto comp = MakePtr<Computer>(Data, std::move(cpuParams), std::move(cpuPorts));
      return MakePtr<Renderer>(*Data, std::move(comp), std::move(channel));
    }

  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      DataBuilder builder(props);
      if (const auto container = Formats::Chiptune::AY::Parse(rawData, 0, builder))
      {
        props.SetSource(*container);
        builder.FillTimingInfo(params);
        auto data = builder.GetResult();
        return MakePtr<Holder>(std::move(data), std::move(properties));
      }
      return {};
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::AYEMUL
