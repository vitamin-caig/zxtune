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
//common includes
#include <byteorder.h>
#include <tools.h>
#include <logging.h>
//library includes
#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/z80.h>
#include <io/container.h>
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

  template<class R, class T>
  const R* GetPointer(const T& obj, int16_t(T::*Member))
  {
    const int16_t* const field = &(obj.*Member);
    const int16_t offset = fromBE(*field);
    return safe_ptr_cast<const R*>(safe_ptr_cast<const uint8_t*>(field) + offset);
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

    virtual void Write(uint64_t /*tick*/, uint16_t addr, uint8_t data)
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
      , LastRenderTick()
      , Reg()
    {
    }

    void Reset()
    {
      Chip->Reset();
      LastRenderTick = 0;
      Reg = 0;
      Data.clear();
    }

    void SelectRegister(uint_t reg)
    {
      Reg = reg;
    }

    void SetValue(uint64_t cpuTick, uint_t val)
    {
      Data.push_back(Chunk(cpuTick, Reg, val));
    }

    void RenderFrame(const Sound::RenderParameters& sndParams)
    {
      std::for_each(Data.begin(), Data.end(), boost::bind(&AYDataChannel::SendChunk, this, _1, CpuParams->TicksPerFrame(), sndParams.ClocksPerFrame()));
      Data.clear();
      LastRenderTick += sndParams.ClocksPerFrame();
      Devices::AYM::DataChunk stub;
      stub.Tick = LastRenderTick;
      Chip->RenderData(stub);
      Chip->Flush();
    }

    Analyzer::Ptr GetAnalyzer() const
    {
      return AYM::CreateAnalyzer(Chip);
    }
  private:
    struct Chunk
    {
      uint64_t CpuTick;
      uint_t Reg;
      uint_t Value;

      Chunk()
        : CpuTick(), Reg(), Value()
      {
      }

      Chunk(uint64_t tick, uint_t reg, uint_t val)
        : CpuTick(tick), Reg(reg), Value(val)
      {
      }
    };

    void SendChunk(const Chunk& chunk, uint64_t inTicksPerFrame, uint64_t outTicksPerFrame)
    {
      Devices::AYM::DataChunk result;
      result.Mask = 1 << chunk.Reg;
      result.Data[chunk.Reg] = chunk.Value;
      result.Tick = ScaleTick(chunk.CpuTick, inTicksPerFrame, outTicksPerFrame);
      Chip->RenderData(result);
    }

    static uint64_t ScaleTick(uint64_t tick, uint64_t inScale, uint64_t outScale)
    {
      //return tick * outScale / inScale;
      const uint64_t baseTick = outScale * (tick / inScale);
      const uint64_t corrTick = (outScale * (tick % inScale)) / inScale;
      return baseTick + corrTick;
    }
  private:
    const Devices::Z80::ChipParameters::Ptr CpuParams;
    const Devices::AYM::Chip::Ptr Chip;
    uint64_t LastRenderTick;
    uint_t Reg;
    std::vector<Chunk> Data;
  };

  class Ports : public Devices::Z80::ChipIO
  {
  public:
    explicit Ports(AYDataChannel::Ptr ayData)
      : AyData(ayData)
    {
    }

    virtual uint8_t Read(uint16_t /*port*/)
    {
      return 0xff;
    }

    virtual void Write(uint64_t tick, uint16_t port, uint8_t data)
    {
      if (0xc0fd == (port & 0xc0ff))
      {
        AyData->SelectRegister(data);
      }
      else if (0x80fd == (port & 0xc0ff))
      {
        AyData->SetValue(tick, data);
      }
    }
  private:
    const AYDataChannel::Ptr AyData;
  };

  class CPUParameters : public Devices::Z80::ChipParameters
  {
  public:
    explicit CPUParameters(Parameters::Accessor::Ptr params)
      : Params(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t TicksPerFrame() const
    {
      const uint64_t clock = 3500000;
      const uint_t frameDuration = Params->FrameDurationMicrosec();
      return static_cast<uint_t>(clock * frameDuration / 1000000); 
    }
  private:
    const Sound::RenderParameters::Ptr Params;
  };

  class AYRenderer : public Renderer
  {
  public:
    AYRenderer(StateIterator::Ptr iterator, Devices::Z80::Chip::Ptr cpu, AYDataChannel::Ptr device)
      : Iterator(iterator)
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

    virtual bool RenderFrame(const Sound::RenderParameters& params)
    {
      CPU->NextFrame();
      Device->RenderFrame(params);
      return Iterator->NextFrame(params.Looped());
    }

    virtual void Reset()
    {
      Iterator->Reset();
      CPU->Reset();
      Device->Reset();
    }

    virtual void SetPosition(uint_t /*frame*/)
    {
    }
  private:
    const StateIterator::Ptr Iterator;
    const Devices::Z80::Chip::Ptr CPU;
    const AYDataChannel::Ptr Device;
    const TrackState::Ptr State;
  };

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

  class AYData
  {
  public:
    typedef boost::shared_ptr<const AYData> Ptr;

    AYData()
      : Data(65536)
      , Frames()
      , Registers()
      , StackPointer()
    {
      InitializeBlock(0xc9, 0, 0x100);
      InitializeBlock(0xff, 0x100, 0x3f00);
      InitializeBlock(uint8_t(0x00), 0x4000, 0xc000);
      Data[0x38] = 0xfb;
    }

    std::size_t ParseInfo(uint_t idx, const IO::FastDump& data, ModuleProperties& properties)
    {
      const uint8_t* lastUsed = &data[0];
      const AYHeader* const header = safe_ptr_cast<const AYHeader*>(&data[0]);
      const ModuleDescription* const description = GetPointer<ModuleDescription>(*header, &AYHeader::DescriptionsOffset) + idx;
      {
        const uint8_t* const titleBegin = GetPointer<uint8_t>(*description, &ModuleDescription::TitleOffset);
        const uint8_t* const titleEnd = std::find(titleBegin, &data[0] + data.Size(), 0);
        properties.SetTitle(OptimizeString(String(titleBegin, titleEnd)));
        lastUsed = std::max(lastUsed, titleEnd);
      }
      {
        const uint8_t* const authorBegin = GetPointer<uint8_t>(*header, &AYHeader::AuthorOffset);
        const uint8_t* const authorEnd = std::find(authorBegin, &data[0] + data.Size(), 0);
        properties.SetAuthor(OptimizeString(String(authorBegin, authorEnd)));
        lastUsed = std::max(lastUsed, authorEnd);
      }
      return lastUsed - &data[0];
    }

    std::size_t ParseData(uint_t idx, const IO::FastDump& data)
    {
      const uint8_t* lastUsed = &data[0];
      const AYHeader* const header = safe_ptr_cast<const AYHeader*>(&data[0]);
      const ModuleDescription* const description = GetPointer<ModuleDescription>(*header, &AYHeader::DescriptionsOffset) + idx;
      const ModuleDataEMUL* const moddata = GetPointer<ModuleDataEMUL>(*description, &ModuleDescription::DataOffset);
      Frames = fromBE(moddata->TotalLength);
      Registers = fromBE(moddata->RegValue);
      const ModuleBlockEMUL* block = GetPointer<ModuleBlockEMUL>(*moddata, &ModuleDataEMUL::BlocksOffset);
      {
        const ModulePointersEMUL* const modptrs = GetPointer<ModulePointersEMUL>(*moddata, &ModuleDataEMUL::PointersOffset);
        const std::size_t initRoutine = fromBE(modptrs->InitAddr ? modptrs->InitAddr : block->Address);
        if (std::size_t intRoutine = fromBE(modptrs->PlayAddr))
        {
          Im1Player player(initRoutine, intRoutine);
          InitializeBlock(&player, 0, sizeof(player));
        }
        else
        {
          Im2Player player(initRoutine);
          InitializeBlock(&player, 0, sizeof(player));
        }
        StackPointer = fromBE(modptrs->SP);
      }
      for (; block->Address; ++block)
      {
        const uint8_t* const src = GetPointer<uint8_t>(*block, &ModuleBlockEMUL::Offset);
        const std::size_t offset = src - &data[0];
        if (offset >= data.Size())
        {
          continue;
        }
        const std::size_t size = fromBE(block->Size);
        const std::size_t addr = fromBE(block->Address);
        const std::size_t toCopy = std::min(size, data.Size() - offset);
        InitializeBlock(src, addr, toCopy);
        lastUsed = std::max(lastUsed, src + toCopy);
      }
      return lastUsed - &data[0];
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
  private:
    void InitializeBlock(uint8_t src, std::size_t offset, std::size_t size)
    {
      const std::size_t toFill = std::min(size, Data.size() - offset);
      std::memset(&Data[offset], src, toFill);
    }

    void InitializeBlock(const void* src, std::size_t offset, std::size_t size)
    {
      const std::size_t toCopy = std::min(size, Data.size() - offset);
      std::memcpy(&Data[offset], src, toCopy);
    }
  private:
    Dump Data;
    uint_t Frames;
    uint16_t Registers;
    uint16_t StackPointer;
  };

  class AYHolder : public Holder
  {
  public:
    AYHolder(AYData::Ptr data, ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters)
      : Data(data)
      , Info(CreateStreamInfo(Data->GetFramesCount(), Devices::AYM::CHANNELS))
      , Properties(properties)
      , Params(parameters)
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Params, Properties);
    }

    virtual Renderer::Ptr CreateRenderer(Sound::MultichannelReceiver::Ptr target) const
    {
      const Parameters::Accessor::Ptr params = GetModuleProperties();

      const StateIterator::Ptr iterator = CreateStreamStateIterator(Info);
      const Devices::AYM::Receiver::Ptr receiver = AYM::CreateReceiver(target);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(chipParams, receiver);
      const Devices::Z80::ChipParameters::Ptr cpuParams = boost::make_shared<CPUParameters>(params);
      const AYDataChannel::Ptr ayChannel = boost::make_shared<AYDataChannel>(cpuParams, chip);
      const Devices::Z80::ChipIO::Ptr cpuPorts = boost::make_shared<Ports>(ayChannel);
      const Devices::Z80::Chip::Ptr cpu = Data->CreateCPU(cpuParams, cpuPorts);
      return boost::make_shared<AYRenderer>(iterator, cpu, ayChannel);
    }

    virtual Error Convert(const Conversion::Parameter& spec, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&spec))
      {
        Properties->GetData(dst);
      }
      return result;
    }
  private:
    const AYData::Ptr Data;
    const Information::Ptr Info;
    const ModuleProperties::Ptr Properties;
    const Parameters::Accessor::Ptr Params;
  };

  bool CheckAY(const IO::DataContainer& inputData)
  {
    if (inputData.Size() <= sizeof(AYHeader))
    {
      return false;
    }
    const AYHeader* const header = safe_ptr_cast<const AYHeader*>(inputData.Data());
    return 0 == std::memcmp(header->Signature, AY_SIGNATURE, sizeof(AY_SIGNATURE)) &&
      (0 == std::memcmp(header->Type, TYPE_EMUL, sizeof(TYPE_EMUL)));
  }

  const std::string AY_FORMAT(
    "'Z'X'A'Y" // uint8_t Signature[4];
    "'E'M'U'L" // only one type is supported now
  );


  class AYPlugin : public PlayerPlugin
                 , public ModulesFactory
                 , public boost::enable_shared_from_this<AYPlugin>
  {
  public:
    typedef boost::shared_ptr<const AYPlugin> Ptr;
    
    AYPlugin()
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
      return CheckAY(inputData);
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const AYPlugin::Ptr self = shared_from_this();
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

        const boost::shared_ptr<AYData> result = boost::make_shared<AYData>();
        const IO::FastDump data(*rawData);
        const std::size_t lastInfo = result->ParseInfo(0, data, *properties);
        const std::size_t lastData = result->ParseData(0, data);

        usedSize = std::max(lastInfo, lastData);
        properties->SetSource(usedSize, ModuleRegion(0, usedSize));
        return boost::make_shared<AYHolder>(result, properties, parameters);
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
}

namespace ZXTune
{
  void RegisterAYSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new AYPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
