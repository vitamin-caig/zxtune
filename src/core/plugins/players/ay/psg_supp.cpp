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
//library includes
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune_decoders.h>
#include <formats/chiptune/psg.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  typedef std::vector<Devices::AYM::DataChunk> ChunksArray;

  class ChunksSet
  {
  public:
    typedef boost::shared_ptr<const ChunksSet> Ptr;

    explicit ChunksSet(std::auto_ptr<ChunksArray> data)
      : Data(data)
    {
    }

    std::size_t Count() const
    {
      return Data->size();
    }

    const Devices::AYM::DataChunk& Get(std::size_t frameNum) const
    {
      return Data->at(frameNum);
    }
  private:
    const std::auto_ptr<ChunksArray> Data;  
  };

  class Builder : public Formats::Chiptune::PSG::Builder
  {
  public:
    virtual void AddChunks(std::size_t count)
    {
      if (!Allocate(count))
      {
        Append(count);
      }
    }

    virtual void SetRegister(uint_t reg, uint_t val)
    {
      if (Data.get() && !Data->empty())
      {
        Devices::AYM::DataChunk& chunk = Data->back();
        chunk.Data[reg] = val;
        chunk.Mask |= uint_t(1) << reg;
      }
    }

    ChunksSet::Ptr Result() const
    {
      Allocate(0);
      return ChunksSet::Ptr(new ChunksSet(Data));
    }
  private:
    bool Allocate(std::size_t count) const
    {
      if (!Data.get())
      {
        Data.reset(new ChunksArray(count));
        return true;
      }
      return false;
    }

    void Append(std::size_t count)
    {
      std::fill_n(std::back_inserter(*Data), count, Devices::AYM::DataChunk());
    }
  private:
    mutable std::auto_ptr<ChunksArray> Data;
  };


  class PSGDataIterator : public AYM::DataIterator
  {
  public:
    PSGDataIterator(StateIterator::Ptr delegate, ChunksSet::Ptr data)
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
        const Devices::AYM::DataChunk& inChunk = Data->Get(frameNum);
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
    const ChunksSet::Ptr Data;
    Devices::AYM::DataChunk CurrentChunk;
  };

  class PSGChiptune : public AYM::Chiptune
  {
  public:
    PSGChiptune(ChunksSet::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateStreamInfo(Data->Count(), Devices::AYM::CHANNELS))
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
    const ChunksSet::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'P', 'S', 'G', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();

  class PSGModulesFactory : public ModulesFactory
  {
  public:
    explicit PSGModulesFactory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& data) const
    {
      return Decoder->Check(data);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      using namespace Formats::Chiptune;
      Builder builder;
      if (const Container::Ptr container = PSG::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const ChunksSet::Ptr data = builder.Result();
        if (data->Count())
        {
          const AYM::Chiptune::Ptr chiptune = boost::make_shared<PSGChiptune>(data, properties);
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreatePSGDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<PSGModulesFactory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder->GetDescription() + Text::PLAYER_DESCRIPTION_SUFFIX, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
