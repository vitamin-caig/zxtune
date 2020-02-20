/**
* 
* @file
*
* @brief  AY EMUL chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/aym/ayemul.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_properties_helper.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <debug/log.h>
#include <devices/beeper.h>
#include <devices/z80.h>
#include <formats/chiptune/emulation/ay.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>

namespace Module
{
namespace AYEMUL
{
  const Debug::Stream Dbg("Core::AYSupp");

  class AyDataChannel
  {
  public:
    explicit AyDataChannel(Devices::AYM::Device::Ptr chip)
      : Chip(std::move(chip))
      , Register()
      , Blocked()
    {
    }

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
        const Devices::AYM::Registers::Index idx = static_cast<Devices::AYM::Registers::Index>(Register);
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

    void RenderFrame(const Devices::AYM::Stamp& till)
    {
      AllocateChunk(till);
      Chip->RenderData(Chunks);
      Chunks.clear();
    }

    Analyzer::Ptr GetAnalyzer() const
    {
      return AYM::CreateAnalyzer(Chip);
    }
  private:
    bool IsRegisterSelected() const
    {
      return Register < Devices::AYM::Registers::TOTAL;
    }
    
    Devices::AYM::DataChunk* GetChunk(const Devices::AYM::Stamp& timeStamp)
    {
      return Blocked
        ? nullptr
        : &AllocateChunk(timeStamp);
    }

    Devices::AYM::DataChunk& AllocateChunk(const Devices::AYM::Stamp& timeStamp)
    {
      Chunks.resize(Chunks.size() + 1);          
      Devices::AYM::DataChunk& res = Chunks.back();
      res.TimeStamp = timeStamp;
      return res;
    }
  private:
    const Devices::AYM::Device::Ptr Chip;
    uint_t Register;
    std::vector<Devices::AYM::DataChunk> Chunks;
    Devices::AYM::DataChunk State;
    bool Blocked;
  };
  
  class BeeperDataChannel
  {
  public:
    explicit BeeperDataChannel(Devices::Beeper::Device::Ptr chip)
      : Chip(std::move(chip))
      , State(false)
      , Blocked(false)
    {
    }
    
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
    
    void RenderFrame(const Devices::Beeper::Stamp& till)
    {
      AllocateChunk(till);
      Chip->RenderData(Chunks);
      Chunks.clear();
    }
  private:
    void AllocateChunk(const Devices::Beeper::Stamp& timeStamp)
    {
      Chunks.resize(Chunks.size() + 1);
      Devices::Beeper::DataChunk& chunk = Chunks.back();
      chunk.TimeStamp = timeStamp;
      chunk.Level = State;
    }
  private:
    const Devices::Beeper::Device::Ptr Chip;
    std::vector<Devices::Beeper::DataChunk> Chunks;
    bool State;
    bool Blocked;
  };
  
  class DataChannel
  {
  public:
    typedef std::shared_ptr<DataChannel> Ptr;
    
    DataChannel(Devices::AYM::Device::Ptr ay, Devices::Beeper::Device::Ptr beep)
      : Ay(std::move(ay))
      , Beeper(std::move(beep))
    {
    }

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

    void RenderFrame(const Devices::Z80::Stamp& till)
    {
      Ay.RenderFrame(till.CastTo<Devices::AYM::TimeUnit>());
      Beeper.RenderFrame(till.CastTo<Devices::Beeper::TimeUnit>());
    }

    Analyzer::Ptr GetAnalyzer() const
    {
      return Ay.GetAnalyzer();
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
    {
    }

    void Reset()
    {
      Channel->SelectAyRegister(0);
    }

    uint8_t Read(uint16_t port)
    {
      return IsSelRegPort(port)
        ? Channel->GetAyValue()
        : 0xff;
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
      , Data()
      , Selector()
    {
    }

    void Reset()
    {
      Channel->SelectAyRegister(0);
      Data = 0;
      Selector = 0;
    }

    uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (IsDataPort(port))
      {
        Data = data;
        //data means nothing
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
    uint8_t Data;
    uint_t Selector;
  };

  class PortsPlexer : public Devices::Z80::ChipIO
  {
  public:
    explicit PortsPlexer(DataChannel::Ptr channel)
      : Channel(channel)
      , ZX(channel)
      , CPC(channel)
      , Current()
    {
    }
    typedef std::shared_ptr<PortsPlexer> Ptr;

    static Ptr Create(DataChannel::Ptr ayData)
    {
      return MakePtr<PortsPlexer>(ayData);
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
      //check CPC first
      else if (CPC.Write(timeStamp, port, data))
      {
        Dbg("Detected CPC port mapping on write to port #%1$04x", port);
        Current = &CPC;
      }
      else 
      {
        //ZX is fallback that will never become current :(
        ZX.Write(timeStamp, port, data);
      }
    }
  private:
    const DataChannel::Ptr Channel;
    ZXAYPort ZX;
    CPCAYPort CPC;
    CPCAYPort* Current;
  };

  class CPUParameters : public Devices::Z80::ChipParameters
  {
  public:
    explicit CPUParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {
    }

    uint_t Version() const override
    {
      return Params->Version();
    }
    
    uint_t IntTicks() const override
    {
      using namespace Parameters::ZXTune::Core::Z80;
      Parameters::IntType intTicks = INT_TICKS_DEFAULT;
      Params->FindValue(INT_TICKS, intTicks);
      return static_cast<uint_t>(intTicks);
    }

    uint64_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core;
      Parameters::IntType cpuClock = Z80::CLOCKRATE_DEFAULT;
      Params->FindValue(Z80::CLOCKRATE, cpuClock);
      return static_cast<uint_t>(cpuClock); 
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class ModuleData
  {
  public:
    typedef std::shared_ptr<const ModuleData> Ptr;
    typedef std::shared_ptr<ModuleData> RWPtr;

    ModuleData()
      : Frames()
      , Registers()
      , StackPointer()
    {
    }

    Devices::Z80::Chip::Ptr CreateCPU(Devices::Z80::ChipParameters::Ptr params, Devices::Z80::ChipIO::Ptr ports) const
    {
      const uint8_t* const rawMemory = static_cast<const uint8_t*>(Memory->Start());
      const Devices::Z80::Chip::Ptr result = Devices::Z80::CreateChip(params, Dump(rawMemory, rawMemory + Memory->Size()), ports);
      Devices::Z80::Registers regs;
      regs.Mask = ~0;
      std::fill(regs.Data.begin(), regs.Data.end(), Registers);
      regs.Data[Devices::Z80::Registers::REG_SP] = StackPointer;
      regs.Data[Devices::Z80::Registers::REG_IR] = Registers & 0xff;
      regs.Data[Devices::Z80::Registers::REG_PC] = 0;
      result->SetRegisters(regs);
      return result;
    }

    uint_t Frames;
    uint16_t Registers;
    uint16_t StackPointer;
    Binary::Data::Ptr Memory;
  };

  class Computer
  {
  public:
    typedef std::shared_ptr<Computer> Ptr; 

    Computer(ModuleData::Ptr data, Devices::Z80::ChipParameters::Ptr params, PortsPlexer::Ptr cpuPorts)
      : Data(std::move(data))
      , Params(std::move(params))
      , CPUPorts(std::move(cpuPorts))
      , CPU(Data->CreateCPU(Params, CPUPorts))
    {
    }

    void Reset()
    {
      CPU = Data->CreateCPU(Params, CPUPorts);
      CPUPorts->Reset();
    }

    void NextFrame(const Devices::Z80::Stamp& til)
    {
      CPU->Interrupt();
      CPU->Execute(til);
    }

    void SkipFrames(uint_t count, Time::Duration<Devices::Z80::TimeUnit> frameStep)
    {
      const auto curTime = CPU->GetTime();
      CPUPorts->SetBlocked(true);
      auto pos = curTime;
      for (uint_t frame = 0; frame < count; ++frame)
      {
        pos += frameStep;
        NextFrame(pos);
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
    Renderer(Sound::RenderParameters::Ptr params, StateIterator::Ptr iterator, Computer::Ptr comp, DataChannel::Ptr device)
      : Params(std::move(params))
      , Iterator(std::move(iterator))
      , Comp(std::move(comp))
      , Device(std::move(device))
      , FrameDuration()
      , Looped()
    {
    }

    Module::State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Device->GetAnalyzer();
    }

    bool RenderFrame() override
    {
      try
      {
        if (Iterator->IsValid())
        {
          SynchronizeParameters();
          LastTime += FrameDuration;
          Comp->NextFrame(LastTime);
          Device->RenderFrame(LastTime);
          Iterator->NextFrame(Looped);
        }
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      Params.Reset();
      Iterator->Reset();
      Comp->Reset();
      Device->Reset();
      FrameDuration = {};
      LastTime = {};
      Looped = {};
    }

    void SetPosition(uint_t frameNum) override
    {
      uint_t curFrame = GetState()->Frame();
      if (frameNum < curFrame)
      {
        //rewind
        Iterator->Reset();
        Comp->Reset();
        Device->Reset();
        LastTime = {};
        curFrame = 0;
      }
      SynchronizeParameters();
      uint_t toSkip = 0;
      while (curFrame < frameNum && Iterator->IsValid())
      {
        Iterator->NextFrame({});
        ++curFrame;
        ++toSkip;
      }
      Comp->SkipFrames(toSkip, FrameDuration);
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        FrameDuration = Params->FrameDuration();
        Looped = Params->Looped();
      }
    }
  private:
    Parameters::TrackingHelper<Sound::RenderParameters> Params;
    const StateIterator::Ptr Iterator;
    const Computer::Ptr Comp;
    const DataChannel::Ptr Device;
    Devices::Z80::Stamp LastTime;
    Time::Duration<Devices::Z80::TimeUnit> FrameDuration;
    Sound::LoopParameters Looped;
  };

  class DataBuilder : public Formats::Chiptune::AY::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Data(MakeRWPtr<ModuleData>())
      , Delegate(Formats::Chiptune::AY::CreateMemoryDumpBuilder())
    {
    }

    void SetTitle(String title) override
    {
      Properties.SetTitle(std::move(title));
    }

    void SetAuthor(String author) override
    {
      Properties.SetAuthor(std::move(author));
    }

    void SetComment(String comment) override
    {
      Properties.SetComment(std::move(comment));
    }

    void SetDuration(uint_t duration, uint_t fadeout) override
    {
      Data->Frames = duration;
      if (fadeout)
      {
        /*TODO
        Dbg("Using fadeout of %1% frames", fadeout);
        static const Time::Microseconds FADING_STEP(20000);
        Properties.GetDelegate().SetValue(Parameters::ZXTune::Sound::FADEOUT, FADING_STEP.Get() * fadeout);
        */
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

    ModuleData::Ptr GetResult() const
    {
      Data->Memory = Delegate->Result();
      return Data;
    }
  private:
    PropertiesHelper& Properties;
    const ModuleData::RWPtr Data;
    const Formats::Chiptune::AY::BlobBuilder::Ptr Delegate;
  };
  
  class StubBeeper : public Devices::Beeper::Device
  {
  public:
    void RenderData(const std::vector<Devices::Beeper::DataChunk>& /*src*/) override
    {
    }

    void Reset() override
    {
    }
  };
  
  class BeeperParams : public Devices::Beeper::ChipParameters
  {
  public:
    explicit BeeperParams(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
    {
    }

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint64_t ClockFreq() const override
    {
      const uint_t MIN_Z80_TICKS_PER_OUTPUT = 10;
      using namespace Parameters::ZXTune::Core;
      Parameters::IntType cpuClock = Z80::CLOCKRATE_DEFAULT;
      Params->FindValue(Z80::CLOCKRATE, cpuClock);
      return static_cast<uint_t>(cpuClock) / MIN_Z80_TICKS_PER_OUTPUT;
    }

    uint_t SoundFreq() const override
    {
      return SoundParams->SoundFreq();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  Devices::Beeper::Device::Ptr CreateBeeper(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    auto beeperParams = MakePtr<BeeperParams>(std::move(params));
    return Devices::Beeper::CreateChip(std::move(beeperParams), std::move(target));
  }
  
  class MergedSoundReceiver : public Sound::Receiver
  {
  public:
    explicit MergedSoundReceiver(Sound::Receiver::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }
    
    void ApplyData(Sound::Chunk chunk) override
    {
      if (!Storage.empty())
      {
        if (chunk.size() <= Storage.size())
        {
          std::transform(chunk.begin(), chunk.end(), Storage.begin(), Storage.begin(), &AvgSample);
          Delegate->ApplyData(std::move(Storage));
        }
        else
        {
          std::transform(Storage.begin(), Storage.end(), chunk.begin(), chunk.begin(), &AvgSample);
          Delegate->ApplyData(std::move(chunk));
        }
        Delegate->Flush();
        Storage.clear();
      }
      else
      {
        Storage = std::move(chunk);
      }
    }
    
    void Flush() override
    {
    }
  private:
    static inline Sound::Sample::Type Avg(Sound::Sample::Type lh, Sound::Sample::Type rh)
    {
      return (Sound::Sample::WideType(lh) + rh) / 2;
    }
  
    static inline Sound::Sample AvgSample(Sound::Sample lh, Sound::Sample rh)
    {
      return Sound::Sample(Avg(lh.Left(), rh.Left()), Avg(lh.Right(), rh.Right()));
    }
  private:
    const Sound::Receiver::Ptr Delegate;
    Sound::Chunk Storage;
  };

  class Holder : public AYM::Holder
  {
  public:
    Holder(ModuleData::Ptr data, Information::Ptr info, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Info(std::move(info))
      , Properties(std::move(properties))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      auto mixer = MakePtr<MergedSoundReceiver>(std::move(target));
      auto aym = AYM::CreateChip(params, mixer);
      auto beeper = CreateBeeper(params, std::move(mixer));
      return CreateRenderer(std::move(params), std::move(aym), std::move(beeper));
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const override
    {
      return CreateRenderer(std::move(params), std::move(chip), MakePtr<StubBeeper>());
    }

    AYM::Chiptune::Ptr GetChiptune() const override
    {
      return AYM::Chiptune::Ptr();
    }
  private:
    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr ay, Devices::Beeper::Device::Ptr beep) const
    {
      auto iterator = CreateStreamStateIterator(Info);
      auto cpuParams = MakePtr<CPUParameters>(params);
      auto channel = MakePtr<DataChannel>(std::move(ay), std::move(beep));
      auto cpuPorts = PortsPlexer::Create(channel);
      auto comp = MakePtr<Computer>(Data, std::move(cpuParams), std::move(cpuPorts));
      auto renderParams = Sound::RenderParameters::Create(std::move(params));
      return MakePtr<Renderer>(std::move(renderParams), std::move(iterator), std::move(comp), std::move(channel));
    }
  private:
    const ModuleData::Ptr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      assert(Formats::Chiptune::AY::GetModulesCount(rawData) == 1);

      PropertiesHelper props(*properties);
      DataBuilder builder(props);
      if (const auto container = Formats::Chiptune::AY::Parse(rawData, 0, builder))
      {
        props.SetSource(*container);
        auto data = builder.GetResult();
        const uint_t frames = data->Frames ? data->Frames : GetDurationInFrames(params);
        return MakePtr<Holder>(std::move(data), CreateStreamInfo(frames), properties);
      }
      return Holder::Ptr();
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
