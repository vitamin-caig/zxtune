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
#include "ay_conversion.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
#include "core/src/core.h"
//common includes
#include <byteorder.h>
#include <format.h>
#include <tools.h>
#include <logging.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <devices/z80.h>
#include <io/container.h>
#include <sound/sound_parameters.h>
//std includes
#include <list>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 6CDE76E4

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char AY_PLUGIN_ID[] = {'A', 'Y', 0};
  const String AY_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint8_t AY_SIGNATURE[] = {'Z', 'X', 'A', 'Y'};
  const uint8_t TYPE_EMUL[] = {'E', 'M', 'U', 'L'};

  template<class T>
  const T* GetPointer(const int16_t* field)
  {
    const int16_t offset = fromBE(*field);
    return safe_ptr_cast<const T*>(safe_ptr_cast<const uint8_t*>(field) + offset);
  }

  template<class T>
  void SetPointer(int16_t* ptr, const T obj)
  {
    BOOST_STATIC_ASSERT(boost::is_pointer<T>::value);
    const std::ptrdiff_t offset = safe_ptr_cast<const uint8_t*>(obj) - safe_ptr_cast<const uint8_t*>(ptr);
    assert(offset > 0);//layout data sequentally
    *ptr = fromBE<int16_t>(offset);
  }

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct AYHeader
  {
    uint8_t Signature[4];//ZXAY
    uint8_t Type[4];
    uint8_t FileVersion;
    uint8_t PlayerVersion;
    int16_t SpecialPlayerOffset;
    int16_t AuthorOffset;
    int16_t MiscOffset;
    uint8_t LastModuleIndex;
    uint8_t FirstModuleIndex;
    int16_t DescriptionsOffset;
  } PACK_POST;

  PACK_PRE struct ModuleDescription
  {
    int16_t TitleOffset;
    int16_t DataOffset;
  } PACK_POST;

  PACK_PRE struct ModuleDataEMUL
  {
    uint8_t AmigaChannelsMapping[4];
    uint16_t TotalLength;
    uint16_t FadeLength;
    uint16_t RegValue;
    int16_t PointersOffset;
    int16_t BlocksOffset;
  } PACK_POST;

  PACK_PRE struct ModulePointersEMUL
  {
    uint16_t SP;
    uint16_t InitAddr;
    uint16_t PlayAddr;
  } PACK_POST;

  PACK_PRE struct ModuleBlockEMUL
  {
    uint16_t Address;
    uint16_t Size;
    int16_t Offset;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(AYHeader) == 0x14);
                                                     
  class Memory : public Devices::Z80::ChipIO
  {
  public:
    explicit Memory(const Dump& data)
      : Data(data)
    {
    }

    virtual uint8_t Read(uint16_t addr)
    {
      return Data[addr];
    }
    
    virtual void Write(const Time::NanosecOscillator& /*timeStamp*/, uint16_t addr, uint8_t data)
    {
      Data[addr] = data;
    }
  private:
    Dump Data;
  };

  class AYDataChannel
  {
  public:
    typedef boost::shared_ptr<AYDataChannel> Ptr;

    AYDataChannel(Devices::Z80::ChipParameters::Ptr cpuParams, Devices::AYM::Chip::Ptr chip)
      : CpuParams(cpuParams)
      , Chip(chip)
      , Register()
    {
    }

    void Reset()
    {
      Chip->Reset();
      Register = 0;
      FrameStub = Devices::AYM::DataChunk();
    }

    void SelectRegister(uint_t reg)
    {
      Register = reg;
    }

    bool SetValue(const Time::Nanoseconds& timeStamp, uint_t val)
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

    void SetBeeper(const Time::Nanoseconds& timeStamp, uint_t val)
    {
      Devices::AYM::DataChunk data;
      data.TimeStamp = timeStamp;
      data.Mask = 1 << Devices::AYM::DataChunk::REG_BEEPER;
      data.Data[Devices::AYM::DataChunk::REG_BEEPER] = val;
      Chip->RenderData(data);
    }

    void RenderFrame(const Time::Nanoseconds& till)
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
    const Devices::Z80::ChipParameters::Ptr CpuParams;
    const Devices::AYM::Chip::Ptr Chip;
    uint_t Register;
    Devices::AYM::DataChunk FrameStub;
  };

  class SoundPort
  {
  public:
    typedef boost::shared_ptr<SoundPort> Ptr;

    virtual ~SoundPort() {}

    virtual bool Write(const Time::NanosecOscillator& timeStamp, uint16_t port, uint8_t data) = 0;
  };

  class ZXAYPort : public SoundPort
  {
  public:
    explicit ZXAYPort(AYDataChannel::Ptr ayData)
      : AyData(ayData)
    {
    }

    virtual bool Write(const Time::NanosecOscillator& timeStamp, uint16_t port, uint8_t data)
    {
      if (IsSelRegPort(port))
      {
        AyData->SelectRegister(data);
        return true;
      }
      else if (IsSetValPort(port))
      {
        return AyData->SetValue(timeStamp.GetCurrentTime(), data);
      }
      else if (IsBeeperPort(port))
      {
        static const uint_t REG_VALUES[] = {0, 14, 14, 15};
        const uint_t value = REG_VALUES[(data & 24) >> 3];
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

    virtual bool Write(const Time::NanosecOscillator& timeStamp, uint16_t port, uint8_t data)
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
            AyData->SelectRegister(Data);
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
    uint_t Data;
    uint_t Selector;
  };

  class PlexerPorts : public Devices::Z80::ChipIO
  {
  public:
    explicit PlexerPorts(AYDataChannel::Ptr ayData)
      : ZX(boost::make_shared<ZXAYPort>(ayData))
      , CPC(boost::make_shared<CPCAYPort>(ayData))
    {
    }

    virtual uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    virtual void Write(const Time::NanosecOscillator& timeStamp, uint16_t port, uint8_t data)
    {
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
  };

  class CPUParameters : public Devices::Z80::ChipParameters
  {
  public:
    explicit CPUParameters(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }
    
    virtual uint_t IntTicks() const
    {
      using namespace Parameters::ZXTune::Core::Z80;
      Parameters::IntType intTicks = INT_TICKS_DEFAULT;
      Params->FindIntValue(INT_TICKS, intTicks);
      return static_cast<uint_t>(intTicks);
    }

    virtual uint64_t ClockFreq() const
    {
      using namespace Parameters::ZXTune::Core;
      Parameters::IntType cpuClock = Z80::CLOCKRATE_DEFAULT;
      Params->FindIntValue(Z80::CLOCKRATE, cpuClock);
      return static_cast<uint_t>(cpuClock); 
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class AYRenderer : public Renderer
  {
  public:
    AYRenderer(AYM::TrackParameters::Ptr params, StateIterator::Ptr iterator, Devices::Z80::Chip::Ptr cpu, AYDataChannel::Ptr device)
      : Params(params)
      , Iterator(iterator)
      , CPU(cpu)
      , Device(device)
      , State(Iterator->GetStateObserver())
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
      LastTime += Time::Microseconds(Params->FrameDurationMicrosec());
      CPU->Interrupt();
      CPU->Execute(LastTime);
      Device->RenderFrame(LastTime);
      return Iterator->NextFrame(Params->Looped());
    }

    virtual void Reset()
    {
      Iterator->Reset();
      CPU->Reset();
      Device->Reset();
      LastTime = Time::Nanoseconds();
    }

    virtual void SetPosition(uint_t /*frame*/)
    {
    }
  private:
    const AYM::TrackParameters::Ptr Params;
    const StateIterator::Ptr Iterator;
    const Devices::Z80::Chip::Ptr CPU;
    const AYDataChannel::Ptr Device;
    const TrackState::Ptr State;
    Time::Nanoseconds LastTime;
  };

  class AYDataTarget
  {
  public:
    virtual ~AYDataTarget() {}

    virtual void SetTitle(const String& title) = 0;
    virtual void SetAuthor(const String& author) = 0;
    virtual void SetComment(const String& comment) = 0;
    virtual void SetDuration(uint_t duration) = 0;
    virtual void SetRegisters(uint_t reg, uint_t sp) = 0;
    virtual void SetRoutines(uint_t init, uint_t play) = 0;
    virtual void AddBlock(uint_t addr, const void* data, std::size_t size) = 0;
  };

  std::size_t ParseAY(uint_t idx, const IO::FastDump& data, AYDataTarget& target)
  {
    const uint8_t* lastUsed = &data[0];
    const uint8_t* const limit = &data[0] + data.Size();
    const AYHeader& header = *safe_ptr_cast<const AYHeader*>(&data[0]);
    if (idx > header.LastModuleIndex)
    {
      return 0;
    }
    const ModuleDescription& description = GetPointer<ModuleDescription>(&header.DescriptionsOffset)[idx];
    {
      const uint8_t* const titleBegin = GetPointer<uint8_t>(&description.TitleOffset);
      const uint8_t* const titleEnd = std::find(titleBegin, limit, 0);
      target.SetTitle(OptimizeString(String(titleBegin, titleEnd)));
      lastUsed = std::max(lastUsed, titleEnd);
    }
    {
      const uint8_t* const authorBegin = GetPointer<uint8_t>(&header.AuthorOffset);
      const uint8_t* const authorEnd = std::find(authorBegin, limit, 0);
      target.SetAuthor(OptimizeString(String(authorBegin, authorEnd)));
      lastUsed = std::max(lastUsed, authorEnd);
    }
    {
      const uint8_t* const miscBegin = GetPointer<uint8_t>(&header.MiscOffset);
      const uint8_t* const miscEnd = std::find(miscBegin, limit, 0);
      target.SetComment(OptimizeString(String(miscBegin, miscEnd)));
      lastUsed = std::max(lastUsed, miscEnd);
    }
    const ModuleDataEMUL& moddata = *GetPointer<ModuleDataEMUL>(&description.DataOffset);
    if (moddata.TotalLength)
    {
      target.SetDuration(fromBE(moddata.TotalLength));
    }
    const ModuleBlockEMUL* block = GetPointer<ModuleBlockEMUL>(&moddata.BlocksOffset);
    {
      const ModulePointersEMUL& modptrs = *GetPointer<ModulePointersEMUL>(&moddata.PointersOffset);
      target.SetRegisters(fromBE(moddata.RegValue), fromBE(modptrs.SP));
      target.SetRoutines(fromBE(modptrs.InitAddr ? modptrs.InitAddr : block->Address), fromBE(modptrs.PlayAddr));
      for (; block->Address; ++block)
      {
        const uint8_t* const src = GetPointer<uint8_t>(&block->Offset);
        const std::size_t offset = src - &data[0];
        if (offset >= data.Size())
        {
          continue;
        }
        const std::size_t size = fromBE(block->Size);
        const std::size_t addr = fromBE(block->Address);
        const std::size_t toCopy = std::min(size, data.Size() - offset);
        target.AddBlock(addr, src, toCopy);
        lastUsed = std::max(lastUsed, src + toCopy);
      }
    }
    return lastUsed - &data[0];
  }

  const uint8_t IM1_PLAYER_TEMPLATE[] = 
  {
    0xf3, //di
    0xcd, 0, 0, //call init (+2)
    0xed, 0x56, //loop: im 1
    0xfb, //ei
    0x76, //halt
    0xcd, 0, 0, //call routine (+9)
    0x18, 0xf7 //jr loop
  };

  const uint8_t IM2_PLAYER_TEMPLATE[] = 
  {
    0xf3, //di
    0xcd, 0, 0, //call init (+2)
    0xed, 0x5e, //loop: im 2
    0xfb, //ei
    0x76, //halt
    0x18, 0xfa //jr loop
  };

  class Im1Player
  {
  public:
    Im1Player(uint16_t init, uint16_t introutine)
    {
      std::copy(IM1_PLAYER_TEMPLATE, ArrayEnd(IM1_PLAYER_TEMPLATE), Data.begin());
      Data[0x2] = init & 0xff;
      Data[0x3] = init >> 8;
      Data[0x9] = introutine & 0xff; 
      Data[0xa] = introutine >> 8; //call routine
    }
  private:
    boost::array<uint8_t, 13> Data;
  };

  BOOST_STATIC_ASSERT(sizeof(Im1Player) == sizeof(IM1_PLAYER_TEMPLATE));

  class Im2Player
  {
  public:
    explicit Im2Player(uint16_t init)
    {
      std::copy(IM2_PLAYER_TEMPLATE, ArrayEnd(IM2_PLAYER_TEMPLATE), Data.begin());
      Data[0x2] = init & 0xff;
      Data[0x3] = init >> 8;
    }
  private:
    boost::array<uint8_t, 10> Data;
  };

  BOOST_STATIC_ASSERT(sizeof(Im2Player) == sizeof(IM2_PLAYER_TEMPLATE));


  class AYData : public AYDataTarget
  {
  public:
    typedef boost::shared_ptr<const AYData> Ptr;

    AYData(ModuleProperties::RWPtr properties, uint_t defaultDuration)
      : Properties(properties)
      , Data(65536)
      , Frames(defaultDuration)
      , Registers()
      , StackPointer()
    {
      InitializeBlock(0xc9, 0, 0x100);
      InitializeBlock(0xff, 0x100, 0x3f00);
      InitializeBlock(uint8_t(0x00), 0x4000, 0xc000);
      Data[0x38] = 0xfb;
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
      const Devices::Z80::ChipIO::Ptr memory(new Memory(Data));
      const Devices::Z80::Chip::Ptr result = Devices::Z80::CreateChip(params, memory, ports);
      Devices::Z80::Registers state;
      state.Mask = ~0;
      std::fill(state.Data.begin(), state.Data.end(), Registers);
      state.Data[Devices::Z80::Registers::REG_SP] = StackPointer;
      state.Data[Devices::Z80::Registers::REG_IR] = Registers & 0xff;
      state.Data[Devices::Z80::Registers::REG_PC] = 0;
      result->SetState(state);
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

    virtual void SetDuration(uint_t duration)
    {
      Frames = duration;
    }

    virtual void SetRegisters(uint_t reg, uint_t sp)
    {
      Registers = reg;
      StackPointer = sp;
    }

    virtual void SetRoutines(uint_t init, uint_t play)
    {
      assert(init);
      if (play)
      {
        Im1Player player(init, play);
        AddBlock(0, &player, sizeof(player));
      }
      else
      {
        Im2Player player(init);
        AddBlock(0, &player, sizeof(player));
      }
    }

    virtual void AddBlock(uint_t addr, const void* src, std::size_t size)
    {
      const std::size_t toCopy = std::min(size, Data.size() - addr);
      std::memcpy(&Data[addr], src, toCopy);
    }

  private:
    void InitializeBlock(uint8_t src, std::size_t offset, std::size_t size)
    {
      const std::size_t toFill = std::min(size, Data.size() - offset);
      std::memset(&Data[offset], src, toFill);
    }
  private:
    const ModuleProperties::RWPtr Properties;
    Dump Data;
    uint_t Frames;
    uint16_t Registers;
    uint16_t StackPointer;
  };

  class AYHolder : public Holder
  {
  public:
    AYHolder(AYData::Ptr data, Parameters::Accessor::Ptr parameters)
      : Data(data)
      , Info(CreateStreamInfo(Data->GetFramesCount(), Devices::AYM::CHANNELS))
      , Params(parameters)
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Data->GetProperties()->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Params, Data->GetProperties());
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::MultichannelReceiver::Ptr target) const
    {
      const StateIterator::Ptr iterator = CreateStreamStateIterator(Info);
      const Devices::AYM::Receiver::Ptr receiver = AYM::CreateReceiver(target);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(chipParams, receiver);
      const Devices::Z80::ChipParameters::Ptr cpuParams = boost::make_shared<CPUParameters>(params);
      const AYDataChannel::Ptr ayChannel = boost::make_shared<AYDataChannel>(cpuParams, chip);
      const Devices::Z80::ChipIO::Ptr cpuPorts = boost::make_shared<PlexerPorts>(ayChannel);
      const Devices::Z80::Chip::Ptr cpu = Data->CreateCPU(cpuParams, cpuPorts);
      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      return boost::make_shared<AYRenderer>(trackParams, iterator, cpu, ayChannel);
    }

    virtual Error Convert(const Conversion::Parameter& spec, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&spec))
      {
        Data->GetProperties()->GetData(dst);
      }
      return result;
    }
  private:
    const AYData::Ptr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Params;
  };

  bool CheckAYModule(const IO::DataContainer& inputData)
  {
    if (inputData.Size() <= sizeof(AYHeader))
    {
      return false;
    }
    const AYHeader* const header = safe_ptr_cast<const AYHeader*>(inputData.Data());
    if (0 != std::memcmp(header->Signature, AY_SIGNATURE, sizeof(AY_SIGNATURE)))
    {
      return false;
    }
    if (0 != std::memcmp(header->Type, TYPE_EMUL, sizeof(TYPE_EMUL)))
    {
      return false;
    }
    return true;
  }

  const std::string AY_FORMAT(
    "'Z'X'A'Y" // uint8_t Signature[4];
    "'E'M'U'L" // only one type is supported now
  );

  class AYPlayerPlugin : public PlayerPlugin
                       , public ModulesFactory
                       , public boost::enable_shared_from_this<AYPlayerPlugin>
  {
  public:
    typedef boost::shared_ptr<const AYPlayerPlugin> Ptr;
    
    AYPlayerPlugin()
      : Format(DataFormat::Create(AY_FORMAT))
    {
    }

    virtual String Id() const
    {
      return AY_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::AY_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return AY_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckAYModule(inputData);
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const AYPlayerPlugin::Ptr self = shared_from_this();
      return DetectModuleInLocation(self, self, inputData, callback);
    }
  private:
    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr rawData, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*rawData));

        Parameters::IntType defaultDuration = Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES_DEFAULT;
        parameters->FindIntValue(Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES, defaultDuration);

        const boost::shared_ptr<AYData> result = boost::make_shared<AYData>(properties, static_cast<uint_t>(defaultDuration));
        const IO::FastDump data(*rawData);
        usedSize = ParseAY(0, data, *result);
        properties->SetSource(usedSize, ModuleRegion(0, usedSize));
        return boost::make_shared<AYHolder>(result, parameters);
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::AYSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };

  //as a container
  class VariableDump : public Dump
  {
  public:
    VariableDump()
    {
      reserve(1000000);
    }

    template<class T>
    T* Add(const T& obj)
    {
      return static_cast<T*>(Add(&obj, sizeof(obj)));
    }

    char* Add(const String& str)
    {
      return static_cast<char*>(Add(str.c_str(), str.size() + 1));
    }

    void* Add(const void* src, std::size_t srcSize)
    {
      const std::size_t prevSize = size();
      resize(prevSize + srcSize);
      void* const dst = &front() + prevSize;
      std::memcpy(dst, src, srcSize);
      return dst;
    }
  };

  class AYBuilder : public AYDataTarget
  {
  public:
    AYBuilder()
      : Duration()
      , Register(), StackPointer()
      , InitRoutine(), PlayRoutine()
      , Memory(65536)
    {
    }

    virtual void SetTitle(const String& title)
    {
      Title = title;
    }

    virtual void SetAuthor(const String& author)
    {
      Author = author;
    }

    virtual void SetComment(const String& comment)
    {
      Comment = comment;
    }

    virtual void SetDuration(uint_t duration)
    {
      Duration = duration;
    }

    virtual void SetRegisters(uint_t reg, uint_t sp)
    {
      Register = reg;
      StackPointer = sp;
    }

    virtual void SetRoutines(uint_t init, uint_t play)
    {
      InitRoutine = init;
      PlayRoutine = play;
    }

    virtual void AddBlock(uint_t addr, const void* data, std::size_t size)
    {
      const std::size_t toCopy = std::min(size, Memory.size() - addr);
      std::memcpy(&Memory[addr], data, toCopy);
      Blocks.push_back(BlocksList::value_type(addr, toCopy));
    }

    IO::DataContainer::Ptr GetAy() const
    {
      VariableDump result;
      //init header
      AYHeader* const header = result.Add(AYHeader());
      std::copy(AY_SIGNATURE, ArrayEnd(AY_SIGNATURE), header->Signature);
      std::copy(TYPE_EMUL, ArrayEnd(TYPE_EMUL), header->Type);
      SetPointer(&header->AuthorOffset, result.Add(Author));
      SetPointer(&header->MiscOffset, result.Add(Comment));
      //init descr
      ModuleDescription* const descr = result.Add(ModuleDescription());
      SetPointer(&header->DescriptionsOffset, descr);
      SetPointer(&descr->TitleOffset, result.Add(Title));
      //init data
      ModuleDataEMUL* const data = result.Add(ModuleDataEMUL());
      SetPointer(&descr->DataOffset, data);
      data->TotalLength = fromBE(Duration);
      data->RegValue = fromBE(Register);
      //init pointers
      ModulePointersEMUL* const pointers = result.Add(ModulePointersEMUL());
      SetPointer(&data->PointersOffset, pointers);
      pointers->SP = fromBE(StackPointer);
      pointers->InitAddr = fromBE(InitRoutine);
      pointers->PlayAddr = fromBE(PlayRoutine);
      //init blocks
      std::list<ModuleBlockEMUL*> blockPtrs;
      for (BlocksList::const_iterator it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it)
      {
        blockPtrs.push_back(result.Add(ModuleBlockEMUL()));
      }
      result.Add(ModuleBlockEMUL());//limiter
      SetPointer(&data->BlocksOffset, blockPtrs.front());
      //fill blocks
      for (BlocksList::const_iterator it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it, blockPtrs.pop_front())
      {
        ModuleBlockEMUL* const dst = blockPtrs.front();
        dst->Address = fromBE<uint16_t>(it->first);
        dst->Size = fromBE<uint16_t>(it->second);
        SetPointer(&dst->Offset, result.Add(&Memory[it->first], it->second));
      }
      return IO::CreateDataContainer(result);
    }

    IO::DataContainer::Ptr GetRaw() const
    {
      return IO::CreateDataContainer(Memory);
    }
  private:
    String Title;
    String Author;
    String Comment;
    uint16_t Duration;
    uint16_t Register;
    uint16_t StackPointer;
    uint16_t InitRoutine;
    uint16_t PlayRoutine;
    Dump Memory;
    typedef std::list<std::pair<std::size_t, std::size_t> > BlocksList;
    BlocksList Blocks;
  };

  std::size_t GetAYSubmodules(const IO::DataContainer& inputData)
  {
    if (inputData.Size() <= sizeof(AYHeader))
    {
      return 0;
    }
    const AYHeader* const header = safe_ptr_cast<const AYHeader*>(inputData.Data());
    if (0 != std::memcmp(header->Signature, AY_SIGNATURE, sizeof(AY_SIGNATURE)))
    {
      return 0;
    }
    if (0 != std::memcmp(header->Type, TYPE_EMUL, sizeof(TYPE_EMUL)))
    {
      return 0;
    }
    return header->FirstModuleIndex <= header->LastModuleIndex
      ? header->LastModuleIndex + 1
      : 0;    
  }

  //TODO: extract to common place
  class LoggerHelper
  {
  public:
    LoggerHelper(Log::ProgressCallback* callback, const Plugin& plugin, const String& path)
      : Callback(callback)
      , Id(plugin.Id())
      , Path(path)
      , Current()
    {
    }

    void operator()(const String& cur)
    {
      if (Callback)
      {
        const String text = Path.empty()
          ? Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS_NOPATH, Id, cur)
          : Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS, Id, cur, Path);
        Callback->OnProgress(Current++, text);
      }
    }
  private:
    Log::ProgressCallback* const Callback;
    const String Id;
    const String Path;
    uint_t Current;
  };

  class AYContainerPlugin : public ArchivePlugin
                          , public boost::enable_shared_from_this<AYContainerPlugin>
  {
  public:
    AYContainerPlugin()
      : Format(DataFormat::Create(AY_FORMAT))
      , AyPath(Text::AY_PLUGIN_PREFIX)
      , RawPath(Text::AY_RAW_PLUGIN_PREFIX)
    {
    }

    virtual String Id() const
    {
      return AY_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::AY_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return AY_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MULTITRACK;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      const std::size_t subModules = GetAYSubmodules(*rawData);
      if (subModules < 2)
      {
        return DetectionResult::CreateUnmatched(Format, rawData);
      }

      const Plugin::Ptr plugin = shared_from_this();
      const Log::ProgressCallback::Ptr progress = CreateProgressCallback(callback, subModules);
      const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
      LoggerHelper logger(progress.get(), *plugin, input->GetPath()->AsString());

      std::size_t usedData = 0;
      const IO::FastDump dump(*rawData);
      for (std::size_t idx = 0; idx < subModules; ++idx)
      {
        const String subPath = AyPath.Build(idx);
        AYBuilder builder;
        const std::size_t parsedLimit = ParseAY(idx, dump, builder);
        logger(subPath);
        if (!parsedLimit)
        {
          continue;
        }
        const IO::DataContainer::Ptr subData = builder.GetAy();
        const ZXTune::DataLocation::Ptr subLocation = idx 
          ? CreateNestedLocation(input, subData, plugin, subPath)
          : CreateNestedLocation(input, subData);
        ZXTune::Module::Detect(subLocation, noProgressCallback);
        usedData = std::max(usedData, parsedLimit);
      }
      return usedData
        ? DetectionResult::CreateMatched(usedData)
        : DetectionResult::CreateUnmatched(Format, rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      std::size_t index = 0;
      const bool asRaw = RawPath.GetIndex(pathComp, index);
      if (!asRaw && !AyPath.GetIndex(pathComp, index))
      {
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      const std::size_t subModules = GetAYSubmodules(*inData);
      if (subModules < index)
      {
        return DataLocation::Ptr();
      }
      const IO::FastDump dump(*inData);
      AYBuilder builder;
      if (!ParseAY(index, dump, builder))
      {
        return DataLocation::Ptr();
      }
      const Plugin::Ptr subPlugin = shared_from_this();
      const IO::DataContainer::Ptr subData = asRaw
        ? builder.GetRaw()
        : builder.GetAy();
      return CreateNestedLocation(location, subData, subPlugin, pathComp); 
    }
  private:
    const DataFormat::Ptr Format;
    const IndexPathComponent AyPath;
    const IndexPathComponent RawPath;
  };
}

namespace ZXTune
{
  void RegisterAYSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr playerPlugin(new AYPlayerPlugin());
    const ArchivePlugin::Ptr archivePlugin(new AYContainerPlugin());
    registrator.RegisterPlugin(playerPlugin);
    registrator.RegisterPlugin(archivePlugin);
  }
}
