/**
* 
* @file
*
* @brief  AY modules support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/duration.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/streaming.h"
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <core/conversion/aym.h>
#include <debug/log.h>
#include <devices/beeper.h>
#include <devices/z80.h>
#include <formats/chiptune/emulation/ay.h>
#include <parameters/tracking_helper.h>
#include <sound/sound_parameters.h>
#include <time/oscillator.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const Debug::Stream Dbg("Core::AYSupp");
}

namespace Module
{
namespace AY
{
  class AyDataChannel
  {
  public:
    explicit AyDataChannel(Devices::AYM::Device::Ptr chip)
      : Chip(chip)
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
        ? 0
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
      : Chip(chip)
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
    typedef boost::shared_ptr<DataChannel> Ptr;
    
    DataChannel(Devices::AYM::Device::Ptr ay, Devices::Beeper::Device::Ptr beep)
      : Ay(ay)
      , Beeper(beep)
    {
    }

    static Ptr Create(Devices::AYM::Device::Ptr ay, Devices::Beeper::Device::Ptr beep)
    {
      return boost::make_shared<DataChannel>(ay, beep);
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
      return Ay.SetValue(timeStamp, val);
    }
    
    uint8_t GetAyValue() const
    {
      return Ay.GetValue();
    }
    
    void SetBeeperValue(const Devices::Z80::Stamp& timeStamp, bool val)
    {
      Beeper.SetLevel(timeStamp, val);
    }

    void RenderFrame(const Devices::Z80::Stamp& till)
    {
      Ay.RenderFrame(till);
      Beeper.RenderFrame(till);
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
      : Channel(chan)
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
      : Channel(channel)
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
    typedef boost::shared_ptr<PortsPlexer> Ptr;

    static Ptr Create(DataChannel::Ptr ayData)
    {
      return boost::make_shared<PortsPlexer>(ayData);
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

    virtual uint8_t Read(uint16_t port)
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

    virtual void Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
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
      : Params(params)
    {
    }

    virtual uint_t Version() const
    {
      return Params->Version();
    }
    
    virtual uint_t IntTicks() const
    {
      using namespace Parameters::ZXTune::Core::Z80;
      Parameters::IntType intTicks = INT_TICKS_DEFAULT;
      Params->FindValue(INT_TICKS, intTicks);
      return static_cast<uint_t>(intTicks);
    }

    virtual uint64_t ClockFreq() const
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
    typedef boost::shared_ptr<const ModuleData> Ptr;

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
    typedef boost::shared_ptr<Computer> Ptr; 

    Computer(ModuleData::Ptr data, Devices::Z80::ChipParameters::Ptr params, PortsPlexer::Ptr cpuPorts)
      : Data(data)
      , Params(params)
      , CPUPorts(cpuPorts)
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

    void SkipFrames(uint_t count, const Devices::Z80::Stamp& frameStep)
    {
      const Devices::Z80::Stamp curTime = CPU->GetTime();
      CPUPorts->SetBlocked(true);
      Devices::Z80::Stamp pos = curTime;
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
      : Params(params)
      , Iterator(iterator)
      , Comp(comp)
      , Device(device)
      , State(Iterator->GetStateObserver())
      , FrameDuration()
      , Looped()
    {
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Device->GetAnalyzer();
    }

    virtual bool RenderFrame()
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

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Comp->Reset();
      Device->Reset();
      FrameDuration = LastTime = Devices::Z80::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      uint_t curFrame = State->Frame();
      if (frameNum < curFrame)
      {
        //rewind
        Iterator->Reset();
        Comp->Reset();
        Device->Reset();
        LastTime = Devices::Z80::Stamp();
        curFrame = 0;
      }
      SynchronizeParameters();
      uint_t toSkip = 0;
      while (curFrame < frameNum && Iterator->IsValid())
      {
        Iterator->NextFrame(true);
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
    const TrackState::Ptr State;
    Devices::Z80::Stamp LastTime;
    Devices::Z80::Stamp FrameDuration;
    bool Looped;
  };

  class DataBuilder : public Formats::Chiptune::AY::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Properties(props)
      , Data(boost::make_shared<ModuleData>())
      , Delegate(Formats::Chiptune::AY::CreateMemoryDumpBuilder())
    {
    }

    virtual void SetTitle(const String& title)
    {
      Properties.SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Properties.SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Properties.SetComment(comment);
    }

    virtual void SetDuration(uint_t duration, uint_t fadeout)
    {
      Data->Frames = duration;
      if (fadeout)
      {
        Dbg("Using fadeout of %1% frames", fadeout);
        static const Time::Microseconds FADING_STEP(20000);
        Properties.SetValue(Parameters::ZXTune::Sound::FADEOUT, FADING_STEP.Get() * fadeout);
      }
    }

    virtual void SetRegisters(uint16_t reg, uint16_t sp)
    {
      Data->Registers = reg;
      Data->StackPointer = sp;
    }

    virtual void SetRoutines(uint16_t init, uint16_t play)
    {
      Delegate->SetRoutines(init, play);
    }

    virtual void AddBlock(uint16_t addr, const void* src, std::size_t size)
    {
      Delegate->AddBlock(addr, src, size);
    }

    ModuleData::Ptr GetResult() const
    {
      Data->Memory = Delegate->Result();
      return Data;
    }
  private:
    PropertiesBuilder& Properties;
    const boost::shared_ptr<ModuleData> Data;
    const Formats::Chiptune::AY::BlobBuilder::Ptr Delegate;
  };
  
  class StubBeeper : public Devices::Beeper::Device
  {
  public:
    virtual void RenderData(const std::vector<Devices::Beeper::DataChunk>& /*src*/)
    {
    }

    virtual void Reset()
    {
    }
  };
  
  class BeeperParams : public Devices::Beeper::ChipParameters
  {
  public:
    explicit BeeperParams(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t Version() const
    {
      return Params->Version();
    }

    virtual uint64_t ClockFreq() const
    {
      const uint_t MIN_Z80_TICKS_PER_OUTPUT = 10;
      using namespace Parameters::ZXTune::Core;
      Parameters::IntType cpuClock = Z80::CLOCKRATE_DEFAULT;
      Params->FindValue(Z80::CLOCKRATE, cpuClock);
      return static_cast<uint_t>(cpuClock) / MIN_Z80_TICKS_PER_OUTPUT;
    }

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  Devices::Beeper::Device::Ptr CreateBeeper(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    const Devices::Beeper::ChipParameters::Ptr beeperParams = boost::make_shared<BeeperParams>(params);
    return Devices::Beeper::CreateChip(beeperParams, target);
  }
  
  class MergedSoundReceiver : public Sound::Receiver
  {
  public:
    explicit MergedSoundReceiver(Sound::Receiver::Ptr delegate)
      : Delegate(delegate)
    {
    }
    
    virtual void ApplyData(const Sound::Chunk::Ptr& chunk)
    {
      if (Storage)
      {
        if (chunk->size() <= Storage->size())
        {
          std::transform(chunk->begin(), chunk->end(), Storage->begin(), Storage->begin(), &MaxSample);
          Delegate->ApplyData(Storage);
        }
        else
        {
          std::transform(Storage->begin(), Storage->end(), chunk->begin(), chunk->begin(), &MaxSample);
          Delegate->ApplyData(chunk);
        }
        Delegate->Flush();
        Storage.reset();
      }
      else
      {
        Storage = chunk;
      }
    }
    
    virtual void Flush()
    {
    }
  private:
    static inline Sound::Sample MaxSample(Sound::Sample lh, Sound::Sample rh)
    {
      return Sound::Sample(std::max(lh.Left(), rh.Left()), std::max(lh.Right(), rh.Right()));
    }
  private:
    const Sound::Receiver::Ptr Delegate;
    Sound::Chunk::Ptr Storage;
  };

  class Holder : public AYM::Holder
  {
  public:
    Holder(ModuleData::Ptr data, Information::Ptr info, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Info(info)
      , Properties(properties)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::Receiver::Ptr mixer = boost::make_shared<MergedSoundReceiver>(target);
      const Devices::AYM::Device::Ptr aym = AYM::CreateChip(params, mixer);
      const Devices::Beeper::Device::Ptr beeper = CreateBeeper(params, mixer);
      return CreateRenderer(params, aym, beeper);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
    {
      return CreateRenderer(params, chip, boost::make_shared<StubBeeper>());
    }

    virtual AYM::Chiptune::Ptr GetChiptune() const
    {
      return AYM::Chiptune::Ptr();
    }
  private:
    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr ay, Devices::Beeper::Device::Ptr beep) const
    {
      const StateIterator::Ptr iterator = CreateStreamStateIterator(Info);
      const Devices::Z80::ChipParameters::Ptr cpuParams = boost::make_shared<CPUParameters>(params);
      const DataChannel::Ptr channel = DataChannel::Create(ay, beep);
      const PortsPlexer::Ptr cpuPorts = PortsPlexer::Create(channel);
      const Computer::Ptr comp = boost::make_shared<Computer>(Data, cpuParams, cpuPorts);
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      return boost::make_shared<Renderer>(renderParams, iterator, comp, channel);
    }
  private:
    const ModuleData::Ptr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class Factory : public Module::Factory
  {
  public:
    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, PropertiesBuilder& propBuilder) const
    {
      assert(Formats::Chiptune::AY::GetModulesCount(rawData) == 1);

      DataBuilder builder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::AY::Parse(rawData, 0, builder))
      {
        propBuilder.SetSource(*container);
        const ModuleData::Ptr data = builder.GetResult();
        const uint_t frames = data->Frames ? data->Frames : GetDurationInFrames(params);
        return boost::make_shared<Holder>(data, CreateStreamInfo(frames), propBuilder.GetResult());
      }
      return Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterAYSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'A', 'Y', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910 | Capabilities::Module::Device::BEEPER
      | Module::AYM::GetSupportedFormatConvertors();

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateAYEMULDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::AY::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
