/**
* 
* @file
*
* @brief  TurboSound containers support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "ts_base.h"
#include "core/plugins/enumerator.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/tracking.h"
#include "core/src/callback.h"
//common includes
#include <error.h>
//library includes
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/aym/turbosound.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const Debug::Stream Dbg("Core::TSSupp");
}

namespace Module
{
namespace TS
{
  class DataBuilder : public Formats::Chiptune::TurboSound::Builder
  {
  public:
    explicit DataBuilder(const Binary::Container& data)
      : Data(data)
    {
    }

    virtual void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size)
    {
      if (!(First = LoadChiptune(offset, size)))
      {
        Dbg("Failed to create first module");
      }
    }

    virtual void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size)
    {
      if (!(Second = LoadChiptune(offset, size)))
      {
        Dbg("Failed to create second module");
      }
    }

    bool HasResult() const
    {
      return First && Second;
    }

    AYM::Chiptune::Ptr GetFirst() const
    {
      return First;
    }

    AYM::Chiptune::Ptr GetSecond() const
    {
      return Second;
    }
  private:
    AYM::Chiptune::Ptr LoadChiptune(std::size_t offset, std::size_t size) const
    {
      const Binary::Container::Ptr content = Data.GetSubcontainer(offset, size);
      if (const AYM::Holder::Ptr holder = boost::dynamic_pointer_cast<const AYM::Holder>(Module::Open(*content)))
      {
        return holder->GetChiptune();
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }

    }
  private:
    const Binary::Container& Data;
    AYM::Chiptune::Ptr First;
    AYM::Chiptune::Ptr Second;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::TurboSound::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual Module::Holder::Ptr CreateModule(Module::PropertiesBuilder& properties, const Binary::Container& data) const
    {
      try
      {
        DataBuilder dataBuilder(data);
        if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(data, dataBuilder))
        {
          if (dataBuilder.HasResult())
          {
            properties.SetSource(*container);
            const TurboSound::Chiptune::Ptr chiptune = TurboSound::CreateChiptune(properties.GetResult(),
              dataBuilder.GetFirst(), dataBuilder.GetSecond());
            return TurboSound::CreateHolder(chiptune);
          }
        }
      }
      catch (const Error&)
      {
      }
      return Module::Holder::Ptr();
    }
  private:
    const Formats::Chiptune::TurboSound::Decoder::Ptr Decoder;
  };
}
}

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'S', 0};
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_TURBOSOUND;

    const Formats::Chiptune::TurboSound::Decoder::Ptr decoder = Formats::Chiptune::TurboSound::CreateDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::TS::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
