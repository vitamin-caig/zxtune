/*
Abstract:
  VTX modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/ym.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

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

  Devices::AYM::LayoutType VtxMode2AymLayout(uint_t mode)
  {
    using namespace Devices::AYM;
    switch (mode)
    {
    case 0:
      return LAYOUT_MONO;
    case 1:
      return LAYOUT_ABC;
    case 2:
      return LAYOUT_ACB;
    case 3:
      return LAYOUT_BAC;
    case 4:
      return LAYOUT_BCA;
    case 5:
      return LAYOUT_CAB;
    case 6:
      return LAYOUT_CBA;
    default:
      assert(!"Invalid VTX layout");
      return LAYOUT_ABC;
    }
  }

  class Builder : public Formats::Chiptune::YM::Builder
  {
  public:
    explicit Builder(PropertiesBuilder& props)
      : Props(props)
      , Loop(0)
      , Data(new ChunksArray())
    {
    }

    virtual void SetVersion(const String& version)
    {
      Props.SetVersion(version);
    }

    virtual void SetChipType(bool ym)
    {
      Props.SetValue(Parameters::ZXTune::Core::AYM::TYPE, ym ? 1 : 0);
    }

    virtual void SetStereoMode(uint_t mode)
    {
      Props.SetValue(Parameters::ZXTune::Core::AYM::LAYOUT, VtxMode2AymLayout(mode));
    }

    virtual void SetLoop(uint_t loop)
    {
      Loop = loop;
    }

    virtual void SetDigitalSample(uint_t /*idx*/, const Dump& /*data*/)
    {
      //TODO:
    }

    virtual void SetClockrate(uint64_t freq)
    {
      Props.SetValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, freq);
    }

    virtual void SetIntFreq(uint_t freq)
    {
      Props.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::GetPeriodForFrequency<Time::Microseconds>(freq).Get());
    }

    virtual void SetTitle(const String& title)
    {
      Props.SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Props.SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Props.SetComment(comment);
    }

    virtual void SetYear(uint_t year)
    {
      if (year)
      {
        Props.SetValue(ATTR_DATE, year);
      }
    }

    virtual void SetProgram(const String& /*program*/)
    {
      //TODO
    }

    virtual void SetEditor(const String& editor)
    {
      Props.SetProgram(editor);
    }

    virtual void AddData(const Dump& registers)
    {
      Devices::AYM::DataChunk& chunk = Allocate();
      const uint_t availRegs = std::min<uint_t>(registers.size(), Devices::AYM::DataChunk::REG_ENV + 1);
      for (uint_t reg = 0, mask = 1; reg != availRegs; ++reg, mask <<= 1)
      {
        const uint8_t val = registers[reg];
        if (reg != Devices::AYM::DataChunk::REG_ENV || val != 0xff)
        {
          chunk.Data[reg] = val;
          chunk.Mask |= mask;
        }
      }
    }

    ChunksSet::Ptr GetResult() const
    {
      return ChunksSet::Ptr(new ChunksSet(Data));
    }

    uint_t GetLoop() const
    {
      return Loop;
    }
  private:
    Devices::AYM::DataChunk& Allocate()
    {
      Data->push_back(Devices::AYM::DataChunk());
      return Data->back();
    }
  private:
    PropertiesBuilder& Props;
    uint_t Loop;
    mutable std::auto_ptr<ChunksArray> Data;
  };

  class DataIterator : public AYM::DataIterator
  {
  public:
    DataIterator(StateIterator::Ptr delegate, ChunksSet::Ptr data)
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

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ChunksSet::Ptr data, Parameters::Accessor::Ptr properties, uint_t loopFrame)
      : Data(data)
      , Properties(properties)
      , Info(CreateStreamInfo(Data->Count(), loopFrame))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr /*trackParams*/) const
    {
      const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
      return boost::make_shared<DataIterator>(iter, Data);
    }
  private:
    const ChunksSet::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace VTX
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'V', 'T', 'X', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, Binary::Container::Ptr data) const
    {
      using namespace Formats::Chiptune;
      Builder builder(properties);
      if (const Container::Ptr container = YM::ParseVTX(*data, builder))
      {
        properties.SetSource(container);
        const ChunksSet::Ptr data = builder.GetResult();
        if (data->Count())
        {
          const AYM::Chiptune::Ptr chiptune = boost::make_shared<Chiptune>(data, properties.GetResult(), builder.GetLoop());
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace YM
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'Y', 'M', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, Binary::Container::Ptr data) const
    {
      Builder builder(properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::YM::ParseYM(*data, builder))
      {
        properties.SetSource(container);
        const ChunksSet::Ptr data = builder.GetResult();
        if (data->Count())
        {
          const AYM::Chiptune::Ptr chiptune = boost::make_shared<Chiptune>(data, properties.GetResult(), builder.GetLoop());
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
  void RegisterVTXSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateVTXDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<VTX::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(VTX::ID, decoder->GetDescription(), VTX::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterYMSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateYMDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<YM::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(YM::ID, decoder->GetDescription(), YM::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
