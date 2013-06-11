/*
Abstract:
  AY modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
#include "core/src/core.h"
//common includes
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <core/conversion/aym.h>
#include <debug/log.h>
#include <devices/z80.h>
#include <devices/details/parameters_helper.h>
#include <formats/chiptune/aym/ay.h>
#include <sound/sound_parameters.h>
#include <time/oscillator.h>
//std includes
#include <list>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <formats/text/chiptune.h>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Debug::Stream Dbg("Core::AYSupp");
                                                     
  class AYDataChannel
  {
  public:
    typedef boost::shared_ptr<AYDataChannel> Ptr;

    explicit AYDataChannel(Devices::AYM::Device::Ptr chip)
      : Chip(chip)
      , Register()
    {
    }

    static Ptr Create(Devices::AYM::Device::Ptr chip)
    {
      return boost::make_shared<AYDataChannel>(chip);
    }

    void Reset()
    {
      Chip->Reset();
      Register = 0;
      FrameStub = Devices::AYM::DataChunk();
    }

    bool SelectRegister(uint_t reg)
    {
      Register = reg;
      return Register < Devices::AYM::DataChunk::REG_BEEPER;
    }

    bool SetValue(const Devices::AYM::Stamp& timeStamp, uint8_t val)
    {
      if (Register < Devices::AYM::DataChunk::REG_BEEPER)
      {
        Devices::AYM::DataChunk data;
        data.TimeStamp = timeStamp;
        data.Mask = 1 << Register;
        data.Data[Register] = val;
        Chip->RenderData(data);
        return true;
      }
      return false;
    }

    void SetBeeper(const Devices::AYM::Stamp& timeStamp, uint8_t val)
    {
      Devices::AYM::DataChunk data;
      data.TimeStamp = timeStamp;
      data.Mask = 1 << Devices::AYM::DataChunk::REG_BEEPER;
      data.Data[Devices::AYM::DataChunk::REG_BEEPER] = val;
      Chip->RenderData(data);
    }

    void RenderFrame(const Devices::AYM::Stamp& till)
    {
      FrameStub.TimeStamp = till;
      Chip->RenderData(FrameStub);
      Chip->Flush();
    }

    Analyzer::Ptr GetAnalyzer() const
    {
      return AYM::CreateAnalyzer(Chip);
    }
  private:
    const Devices::AYM::Device::Ptr Chip;
    uint_t Register;
    Devices::AYM::DataChunk FrameStub;
  };

  class SoundPort
  {
  public:
    typedef boost::shared_ptr<SoundPort> Ptr;

    virtual ~SoundPort() {}

    virtual void Reset() = 0;
    virtual bool Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data) = 0;
  };

  class ZXAYPort : public SoundPort
  {
  public:
    explicit ZXAYPort(AYDataChannel::Ptr ayData)
      : AyData(ayData)
    {
    }

    virtual void Reset()
    {
      AyData->SelectRegister(0);
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
        static const uint8_t REG_VALUES[] = {0, 14, 14, 15};
        const uint8_t value = REG_VALUES[(data & 24) >> 3];
        AyData->SetBeeper(timeStamp.GetCurrentTime(), value);
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
    const AYDataChannel::Ptr AyData;
  };

  class CPCAYPort : public SoundPort
  {
  public:
    explicit CPCAYPort(AYDataChannel::Ptr ayData)
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
    const AYDataChannel::Ptr AyData;
    uint8_t Data;
    uint_t Selector;
  };

  class PortsPlexer : public Devices::Z80::ChipIO
  {
  public:
    explicit PortsPlexer(AYDataChannel::Ptr ayData)
      : ZX(boost::make_shared<ZXAYPort>(ayData))
      , CPC(boost::make_shared<CPCAYPort>(ayData))
      , Blocked(false)
    {
    }
    typedef boost::shared_ptr<PortsPlexer> Ptr;

    static Ptr Create(AYDataChannel::Ptr ayData)
    {
      return boost::make_shared<PortsPlexer>(ayData);
    }

    void SetBlocked(bool blocked)
    {
      Blocked = blocked;
    }

    void Reset()
    {
      ZX->Reset();
      CPC->Reset();
    }

    virtual uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    virtual void Write(const Devices::Z80::Oscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (Blocked)
      {
        return;
      }
      if (Current)
      {
        Current->Write(timeStamp, port, data);
      }
      //check CPC first
      else if (CPC->Write(timeStamp, port, data))
      {
        Current = CPC;
      }
      else 
      {
        //ZX is fallback that will never become current :(
        ZX->Write(timeStamp, port, data);
      }
    }
  private:
    const SoundPort::Ptr ZX;
    const SoundPort::Ptr CPC;
    SoundPort::Ptr Current;
    bool Blocked;
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

  class Computer
  {
  public:
    typedef boost::shared_ptr<Computer> Ptr; 

    virtual ~Computer() {}

    virtual void Reset() = 0;
    virtual void NextFrame(const Devices::Z80::Stamp& til) = 0;
    virtual void SeekState(const Devices::Z80::Stamp& til, const Devices::Z80::Stamp& frameStep) = 0;
  };

  class AYRenderer : public Renderer
  {
  public:
    AYRenderer(Sound::RenderParameters::Ptr params, StateIterator::Ptr iterator, Computer::Ptr comp, AYDataChannel::Ptr device)
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
    const AYDataChannel::Ptr Device;
    const TrackState::Ptr State;
    Devices::Z80::Stamp LastTime;
    Devices::Z80::Stamp FrameDuration;
    bool Looped;
  };

  class AYData : public Formats::Chiptune::AY::Builder
  {
  public:
    typedef boost::shared_ptr<const AYData> Ptr;

    AYData(ModuleProperties::RWPtr properties, uint_t defaultDuration)
      : Properties(properties)
      , Delegate(Formats::Chiptune::AY::CreateMemoryDumpBuilder())
      , Frames(defaultDuration)
      , Registers()
      , StackPointer()
    {
    }

    ModuleProperties::Ptr GetProperties() const
    {
      return Properties;
    }

    uint_t GetFramesCount() const
    {
      return Frames;
    }

    Devices::Z80::Chip::Ptr CreateCPU(Devices::Z80::ChipParameters::Ptr params, Devices::Z80::ChipIO::Ptr ports) const
    {
      const Binary::Container::Ptr memory = Delegate->Result();
      const uint8_t* const rawMemory = static_cast<const uint8_t*>(memory->Start());
      const Devices::Z80::Chip::Ptr result = Devices::Z80::CreateChip(params, Dump(rawMemory, rawMemory + memory->Size()), ports);
      Devices::Z80::Registers regs;
      regs.Mask = ~0;
      std::fill(regs.Data.begin(), regs.Data.end(), Registers);
      regs.Data[Devices::Z80::Registers::REG_SP] = StackPointer;
      regs.Data[Devices::Z80::Registers::REG_IR] = Registers & 0xff;
      regs.Data[Devices::Z80::Registers::REG_PC] = 0;
      result->SetRegisters(regs);
      return result;
    }

    //AYDataTarget
    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Properties->SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Properties->SetComment(comment);
    }

    virtual void SetDuration(uint_t duration, uint_t fadeout)
    {
      Frames = duration;
      if (fadeout)
      {
        Dbg("Using fadeout of %1% frames", fadeout);
        static const Time::Microseconds FADING_STEP(20000);
        Properties->GetInternalContainer()->SetValue(Parameters::ZXTune::Sound::FADEOUT, FADING_STEP.Get() * fadeout);
      }
    }

    virtual void SetRegisters(uint16_t reg, uint16_t sp)
    {
      Registers = reg;
      StackPointer = sp;
    }

    virtual void SetRoutines(uint16_t init, uint16_t play)
    {
      Delegate->SetRoutines(init, play);
    }

    virtual void AddBlock(uint16_t addr, const void* src, std::size_t size)
    {
      Delegate->AddBlock(addr, src, size);
    }
  private:
    const ModuleProperties::RWPtr Properties;
    const Formats::Chiptune::AY::BlobBuilder::Ptr Delegate;
    uint_t Frames;
    uint16_t Registers;
    uint16_t StackPointer;
  };

  class ComputerImpl : public Computer
  {
  public:
    ComputerImpl(AYData::Ptr data, Devices::Z80::ChipParameters::Ptr params, PortsPlexer::Ptr cpuPorts)
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
    const AYData::Ptr Data;
    const Devices::Z80::ChipParameters::Ptr Params;
    const PortsPlexer::Ptr CPUPorts;
    Devices::Z80::Chip::Ptr CPU;
  };

  class AYHolder : public AYM::Holder
  {
  public:
    explicit AYHolder(AYData::Ptr data)
      : Data(data)
      , Info(CreateStreamInfo(Data->GetFramesCount()))
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Data->GetProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return AYM::CreateRenderer(*this, params, target);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
    {
      const StateIterator::Ptr iterator = CreateStreamStateIterator(Info);
      const Devices::Z80::ChipParameters::Ptr cpuParams = boost::make_shared<CPUParameters>(params);
      const AYDataChannel::Ptr ayChannel = AYDataChannel::Create(chip);
      const PortsPlexer::Ptr cpuPorts = PortsPlexer::Create(ayChannel);
      const Computer::Ptr comp = boost::make_shared<ComputerImpl>(Data, cpuParams, cpuPorts);
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      return boost::make_shared<AYRenderer>(renderParams, iterator, comp, ayChannel);
    }
  private:
    const AYData::Ptr Data;
    const Information::Ptr Info;
  };
}

namespace AYPlugin
{
  const Char ID[] = {'A', 'Y', 0};
  const Char* const INFO = Text::AY_EMUL_DECODER_DESCRIPTION;
}

namespace AYModule
{
  using namespace ZXTune;

  //plugin attributes
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::SupportedAYMFormatConvertors;

  const std::string HEADER_FORMAT(
    "'Z'X'A'Y" // uint8_t Signature[4];
    "'E'M'U'L" // only one type is supported now
  );

  class Factory : public ModulesFactory
  {
  public:
    Factory()
      : Format(Binary::Format::Create(HEADER_FORMAT))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Formats::Chiptune::AY::GetModulesCount(inputData) == 1;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      try
      {
        assert(Formats::Chiptune::AY::GetModulesCount(*rawData) == 1);

        Parameters::IntType defaultDuration = Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES_DEFAULT;
        //parameters->FindValue(Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES, defaultDuration);

        const boost::shared_ptr<AYData> result = boost::make_shared<AYData>(properties, static_cast<uint_t>(defaultDuration));
        if (Formats::Chiptune::Container::Ptr container = Formats::Chiptune::AY::Parse(*rawData, 0, *result))
        {
          usedSize = container->Size();
          properties->SetSource(container);
          return boost::make_shared<AYHolder>(result);
        }
      }
      catch (const Error&/*e*/)
      {
        Dbg("Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterAYSupport(PlayerPluginsRegistrator& registrator)
  {
    //module
    {
      const ModulesFactory::Ptr factory = boost::make_shared<AYModule::Factory>();
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(AYPlugin::ID, AYPlugin::INFO, AYModule::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
