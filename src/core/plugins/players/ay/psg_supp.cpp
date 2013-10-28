/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base.h"
#include "aym_base_stream.h"
#include "aym_plugin.h"
#include "core/plugins/registrator.h"
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/psg.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace PSG
{
  typedef std::vector<Devices::AYM::Registers> RegistersArray;

  class PSGStreamModel : public AYM::StreamModel
  {
  public:
    explicit PSGStreamModel(RegistersArray& rh)
    {
      Data.swap(rh);
    }

    virtual uint_t Size() const
    {
      return static_cast<uint_t>(Data.size());
    }

    virtual uint_t Loop() const
    {
      return 0;
    }

    virtual Devices::AYM::Registers Get(uint_t pos) const
    {
      return Data[pos];
    }
  private:
    RegistersArray Data;
  };

  class DataBuilder : public Formats::Chiptune::PSG::Builder
  {
  public:
    virtual void AddChunks(std::size_t count)
    {
      Append(count);
    }

    virtual void SetRegister(uint_t reg, uint_t val)
    {
      if (!Data.empty() && reg < Devices::AYM::Registers::TOTAL)
      {
        Devices::AYM::Registers& data = Data.back();
        data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
      }
    }

    AYM::StreamModel::Ptr GetResult() const
    {
      return Data.empty()
        ? AYM::StreamModel::Ptr()
        : AYM::StreamModel::Ptr(new PSGStreamModel(Data));
    }
  private:
    void Append(std::size_t count)
    {
      Data.resize(Data.size() + count);
    }
  private:
    mutable RegistersArray Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    virtual AYM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder;
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::PSG::Parse(rawData, dataBuilder))
      {
        if (const AYM::StreamModel::Ptr data = dataBuilder.GetResult())
        {
          propBuilder.SetSource(*container);
          return AYM::CreateStreamedChiptune(data, propBuilder.GetResult());
        }
      }
      return AYM::Chiptune::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterPSGSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'S', 'G', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreatePSGDecoder();
    const Module::AYM::Factory::Ptr factory = boost::make_shared<Module::PSG::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
