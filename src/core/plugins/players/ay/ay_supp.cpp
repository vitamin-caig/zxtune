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
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/streaming.h"
//library includes
#include <binary/format_factories.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <core/conversion/aym.h>
#include <debug/log.h>
#include <devices/z80.h>
#include <devices/details/parameters_helper.h>
#include <formats/chiptune/aym/ay.h>
#include <sound/sound_parameters.h>
#include <time/oscillator.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Core::AYSupp");
}

namespace Module
{
namespace AY
{
  class DataChannel
  {
  public:
    typedef boost::shared_ptr<DataChannel> Ptr;

    explicit DataChannel(Devices::AYM::Device::Ptr chip)
      : Chip(chip)
      , Register()
      , Blocked()
    {
    }

    static Ptr Create(Devices::AYM::Device::Ptr chip)
    {
      return boost::make_shared<DataChannel>(chip);
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
        if (!Blocked)
        {
          Devices::AYM::DataChunk& chunk = AllocateChunk();
          chunk.TimeStamp = timeStamp;
          chunk.Data[idx] = val;
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

    void SetBeeper(const Devices::AYM::Stamp& timeStamp, bool val)
    {
      if (!Blocked)
      {
        Devices::AYM::DataChunk& chunk = AllocateChunk();
        chunk.TimeStamp = timeStamp;
        chunk.Data.SetBeeper(val);
      }
    }

    void RenderFrame(const Devices::AYM::Stamp& till)
    {
      Devices::AYM::DataChunk& stub = AllocateChunk();
      stub.TimeStamp = till;
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

    Devices::AYM::DataChunk& AllocateChunk()
    {
      Chunks.resize(Chunks.size() + 1);          
      return Chunks.back();
    }
  private:
    const Devices::AYM::Device::Ptr Chip;
    uint_t Register;
    std::vector<Devices::AYM::DataChunk> Chunks;
    Devices::AYM::DataChunk State;
    bool Blocked;
  };

  class SoundPort
  {
  public:
    typedef boost::shared_ptr<SoundPort> Ptr;

    virtual ~SoundPort() {}

    virtual void Reset() = 0;
    virtual uint8_t Read(uint16_t port) = 0;
    virtual bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data) = 0;
  };

  class ZXAYPort : public SoundPort
  {
  public:
    explicit ZXAYPort(DataChannel::Ptr ayData)
      : AyData(ayData)
    {
    }

    virtual void Reset()
    {
      AyData->SelectRegister(0);
    }

    virtual uint8_t Read(uint16_t port)
    {
      return IsSelRegPort(port)
        ? AyData->GetValue()
        : 0xff;
    }

    virtual bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (IsSelRegPort(port))
      {
        return AyData->SelectRegister(data);
      }
      else if (IsSetValPort(port))
      {
        return AyData->SetValue(timeStamp.GetCurrentTime(), data);
      }
      else if (IsBeeperPort(port))
      {
        AyData->SetBeeper(timeStamp.GetCurrentTime(), 0 != (data & 16));
        //beeper means nothing
      }
      return false;
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
    const DataChannel::Ptr AyData;
  };

  class CPCAYPort : public SoundPort
  {
  public:
    explicit CPCAYPort(DataChannel::Ptr ayData)
      : AyData(ayData)
      , Data()
      , Selector()
    {
    }

    virtual void Reset()
    {
      AyData->SelectRegister(0);
      Data = 0;
      Selector = 0;
    }

    virtual uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    virtual bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
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
            res = AyData->SelectRegister(Data);
            break;
          case 0x80:
            res = AyData->SetValue(timeStamp.GetCurrentTime(), Data);
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
    const DataChannel::Ptr AyData;
    uint8_t Data;
    uint_t Selector;
  };

  class PortsPlexer : public Devices::Z80::ChipIO
  {
  public:
    explicit PortsPlexer(DataChannel::Ptr ayData)
      : Data(ayData)
      , ZX(boost::make_shared<ZXAYPort>(ayData))
      , CPC(boost::make_shared<CPCAYPort>(ayData))
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
      ZX->Reset();
      CPC->Reset();
    }

    void SetBlocked(bool blocked)
    {
      Data->SetBlocked(blocked);
    }

    virtual uint8_t Read(uint16_t port)
    {
      if (Current)
      {
        return Current->Read(port);
      }
      else
      {
        return ZX->Read(port);
      }
    }

    virtual void Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (Current)
      {
        Current->Write(timeStamp, port, data);
      }
      //check CPC first
      else if (CPC->Write(timeStamp, port, data))
      {
        Current = CPC.get();
      }
      else 
      {
        //ZX is fallback that will never become current :(
        ZX->Write(timeStamp, port, data);
      }
    }
  private:
    const DataChannel::Ptr Data;
    const SoundPort::Ptr ZX;
    const SoundPort::Ptr CPC;
    SoundPort* Current;
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
      return 1;
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

    void SeekState(const Devices::Z80::Stamp& til, const Devices::Z80::Stamp& frameStep)
    {
      const Devices::Z80::Stamp curTime = CPU->GetTime();
      if (til < curTime)
      {
        Reset();
      }
      CPUPorts->SetBlocked(true);
      Devices::Z80::Stamp pos = curTime;
      while ((pos += frameStep) < til)
      {
        CPU->Interrupt();
        CPU->Execute(pos);
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

    virtual void SetPosition(uint_t frame)
    {
      if (State->Frame() > frame)
      {
        //rewind
        Iterator->Reset();
      }
      SynchronizeParameters();
      Devices::Z80::Stamp newTime = LastTime; 
      while (State->Frame() < frame && Iterator->IsValid())
      {
        newTime += FrameDuration;
        Iterator->NextFrame(false);
      }
      Comp->SeekState(newTime, FrameDuration);
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
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
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
    explicit DataBuilder(PropertiesBuilder& props, uint_t defaultDuration)
      : Properties(props)
      , Data(boost::make_shared<ModuleData>())
      , Delegate(Formats::Chiptune::AY::CreateMemoryDumpBuilder())
    {
      Data->Frames = defaultDuration;
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

  class Holder : public AYM::Holder
  {
  public:
    Holder(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Info(CreateStreamInfo(Data->Frames))
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
      return AYM::CreateRenderer(*this, params, target);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
    {
      const StateIterator::Ptr iterator = CreateStreamStateIterator(Info);
      const Devices::Z80::ChipParameters::Ptr cpuParams = boost::make_shared<CPUParameters>(params);
      const DataChannel::Ptr ayChannel = DataChannel::Create(chip);
      const PortsPlexer::Ptr cpuPorts = PortsPlexer::Create(ayChannel);
      const Computer::Ptr comp = boost::make_shared<Computer>(Data, cpuParams, cpuPorts);
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      return boost::make_shared<Renderer>(renderParams, iterator, comp, ayChannel);
    }

    virtual AYM::Chiptune::Ptr GetChiptune() const
    {
      return AYM::Chiptune::Ptr();
    }
  private:
    const ModuleData::Ptr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  //TODO: extract
  const std::string HEADER_FORMAT(
    "'Z'X'A'Y" // uint8_t Signature[4];
    "'E'M'U'L" // only one type is supported now
    "??"       // versions
    "??"       // player offset
    "??"       // author offset
    "??"       // misc offset
    "00"       // first module
    "00"       // last module
  );
  
  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    Decoder()
      : Format(Binary::CreateFormat(HEADER_FORMAT))
    {
    }

    virtual String GetDescription() const
    {
      return Text::AY_EMUL_DECODER_DESCRIPTION;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual bool Check(const Binary::Container& rawData) const
    {
      return Formats::Chiptune::AY::GetModulesCount(rawData) == 1;
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      return Formats::Chiptune::AY::Parse(rawData, 0, Formats::Chiptune::AY::GetStubBuilder());
    }
  private:
    const Binary::Format::Ptr Format;
  };

  class Factory : public Module::Factory
  {
  public:
    virtual Module::Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      assert(Formats::Chiptune::AY::GetModulesCount(rawData) == 1);

      Parameters::IntType defaultDuration = Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES_DEFAULT;
      //parameters->FindValue(Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES, defaultDuration);

      DataBuilder builder(propBuilder, static_cast<uint_t>(defaultDuration));
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::AY::Parse(rawData, 0, builder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<Holder>(builder.GetResult(), propBuilder.GetResult());
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
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AY38910 | CAP_DEV_BEEPER | CAP_CONV_RAW | Module::AYM::SupportedFormatConvertors;

    const Formats::Chiptune::Decoder::Ptr decoder = boost::make_shared<Module::AY::Decoder>();
    const Module::Factory::Ptr factory = boost::make_shared<Module::AY::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
