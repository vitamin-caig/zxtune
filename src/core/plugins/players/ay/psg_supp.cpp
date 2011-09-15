/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <tools.h>
#include <logging.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const uint8_t PSG_SIGNATURE[] = {'P', 'S', 'G'};

  enum
  {
    PSG_MARKER = 0x1a,

    INT_BEGIN = 0xff,
    INT_SKIP = 0xfe,
    MUS_END = 0xfd
  };

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PSGHeader
  {
    uint8_t Sign[3];
    uint8_t Marker;
    uint8_t Version;
    uint8_t Interrupt;
    uint8_t Padding[10];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PSGHeader) == 16);

  class PSGData
  {
  public:
    typedef boost::shared_ptr<const PSGData> Ptr;

    PSGData()
      : RawDataSize()
    {
    }

    void AddChunks(uint_t count)
    {
      std::fill_n(std::back_inserter(Data), count, Devices::AYM::DataChunk());
    }

    void SetRegister(uint_t reg, uint8_t val)
    {
      if (!Data.empty())
      {
        Devices::AYM::DataChunk& chunk = Data.back();
        chunk.Data[reg] = val;
        chunk.Mask |= uint_t(1) << reg;
      }
    }

    void SetUsedSize(std::size_t usedSize)
    {
      RawDataSize = usedSize;
    }

    std::size_t GetDataSize() const
    {
      return RawDataSize;
    }

    ModuleRegion GetFixedRegion() const
    {
      return ModuleRegion(0, RawDataSize);
    }

    std::size_t GetSize() const
    {
      return Data.size();
    }

    const Devices::AYM::DataChunk& GetChunk(std::size_t frameNum) const
    {
      return Data[frameNum];
    }
  private:
    std::vector<Devices::AYM::DataChunk> Data;
    std::size_t RawDataSize;
  };

  PSGData::Ptr ParsePSG(Binary::Container::Ptr rawData)
  {
    boost::shared_ptr<PSGData> result = boost::make_shared<PSGData>();

    const IO::FastDump data(*rawData);
    //workaround for some emulators
    const std::size_t offset = (data[4] == INT_BEGIN) ? 4 : sizeof(PSGHeader);
    std::size_t size = data.Size() - offset;
    const uint8_t* bdata = &data[offset];
    //detect as much chunks as possible, in despite of real format issues
    while (size)
    {
      const uint_t reg = *bdata;
      ++bdata;
      --size;
      if (INT_BEGIN == reg)
      {
        result->AddChunks(1);
      }
      else if (INT_SKIP == reg)
      {
        if (size < 1)
        {
          ++size;//put byte back
          break;
        }
        result->AddChunks(4 * *bdata);
        ++bdata;
        --size;
      }
      else if (MUS_END == reg)
      {
        break;
      }
      else if (reg <= 15) //register
      {
        if (size < 1)
        {
          ++size;//put byte back
          break;
        }
        result->SetRegister(reg, *bdata);
        ++bdata;
        --size;
      }
      else
      {
        ++size;//put byte back
        break;
      }
    }
    if (!result->GetSize())
    {
      return PSGData::Ptr();
    }
    result->SetUsedSize(data.Size() - size);
    return result;
  }

  class PSGDataIterator : public AYM::DataIterator
  {
  public:
    PSGDataIterator(StateIterator::Ptr delegate, PSGData::Ptr data)
      : Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
    {
      UpdateCurrentState();
    }

    virtual void Reset()
    {
      CurrentChunk = Devices::AYM::DataChunk();
      Delegate->Reset();
      UpdateCurrentState();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      UpdateCurrentState();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual void GetData(Devices::AYM::DataChunk& chunk) const
    {
       chunk = CurrentChunk;
    }
  private:
    void UpdateCurrentState()
    {
      if (Delegate->IsValid())
      {
        const uint_t frameNum = State->Frame();
        const Devices::AYM::DataChunk& inChunk = Data->GetChunk(frameNum);
        ResetEnvelopeChanges();
        for (uint_t reg = 0, mask = inChunk.Mask; mask; ++reg, mask >>= 1)
        {
          if (0 != (mask & 1))
          {
            UpdateRegister(reg, inChunk.Data[reg]);
          }
        }
      }
    }

    void ResetEnvelopeChanges()
    {
      CurrentChunk.Mask &= ~(uint_t(1) << Devices::AYM::DataChunk::REG_ENV);
    }

    void UpdateRegister(uint_t reg, uint8_t data)
    {
      CurrentChunk.Mask |= uint_t(1) << reg;
      CurrentChunk.Data[reg] = data;
    }
  private:
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const PSGData::Ptr Data;
    Devices::AYM::DataChunk CurrentChunk;
  };

  class PSGChiptune : public AYM::Chiptune
  {
  public:
    PSGChiptune(PSGData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateStreamInfo(Data->GetSize(), Devices::AYM::CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual ModuleProperties::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr /*trackParams*/) const
    {
      const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
      return boost::make_shared<PSGDataIterator>(iter, Data);
    }
  private:
    const PSGData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  bool CheckPSG(const Binary::Container& inputData)
  {
    if (inputData.Size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* const header = safe_ptr_cast<const PSGHeader*>(inputData.Data());
    return 0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) &&
       PSG_MARKER == header->Marker;
  }
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'P', 'S', 'G', 0};
  const Char* const INFO = Text::PSG_PLUGIN_INFO;
  const String VERSION(FromStdString("$Rev$"));
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();

  const std::string PSG_FORMAT(
    "'P'S'G" // uint8_t Sign[3];
    "1a"
  );

  class PSGModulesFactory : public ModulesFactory
  {
  public:
    PSGModulesFactory()
      : Format(Binary::Format::Create(PSG_FORMAT))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && CheckPSG(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*data));

        if (const PSGData::Ptr parsedData = ParsePSG(data))
        {
          usedSize = parsedData->GetDataSize();
          properties->SetSource(parsedData->GetDataSize(), parsedData->GetFixedRegion());

          const AYM::Chiptune::Ptr chiptune = boost::make_shared<PSGChiptune>(parsedData, properties);
          return AYM::CreateHolder(chiptune, parameters);
        }
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::PSGSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<PSGModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
