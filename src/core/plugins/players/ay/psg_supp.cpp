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
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <tools.h>
//library includes
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/psg.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace PSG
{
  class DataBuilder : public Formats::Chiptune::PSG::Builder
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
      if (Data.get() && !Data->empty() && reg < Devices::AYM::Registers::TOTAL)
      {
        Devices::AYM::Registers& data = Data->back();
        data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
      }
    }

    AYM::RegistersArrayPtr GetResult() const
    {
      return Data;
    }
  private:
    bool Allocate(std::size_t count) const
    {
      if (!Data.get())
      {
        Data = boost::make_shared<AYM::RegistersArray>(count);
        return true;
      }
      return false;
    }

    void Append(std::size_t count)
    {
      Data->reserve(Data->size() + count);
      std::fill_n(std::back_inserter(*Data), count, Devices::AYM::Registers());
    }
  private:
    mutable boost::shared_ptr<AYM::RegistersArray> Data;
  };

  class Factory : public Module::Factory
  {
  public:
    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder;
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::PSG::Parse(rawData, dataBuilder))
      {
        if (const AYM::RegistersArrayPtr data = dataBuilder.GetResult())
        {
          propBuilder.SetSource(*container);
          const AYM::Chiptune::Ptr chiptune = AYM::CreateStreamedChiptune(data, propBuilder.GetResult(), 0);
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
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
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::AYM::SupportedFormatConvertors;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreatePSGDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::PSG::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
