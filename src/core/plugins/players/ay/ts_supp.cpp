/*
Abstract:
  TS modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ts_base.h"
#include "core/plugins/enumerator.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/tracking.h"
#include "core/src/callback.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/container.h>
#include <sound/mixer_factory.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  const Debug::Stream Dbg("Core::TSSupp");
}

namespace Module
{
namespace TS
{
  const std::size_t TS_MIN_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t TS_MAX_SIZE = MAX_MODULE_SIZE * 2;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Footer
  {
    uint8_t ID1[4];//'PT3!' or other type
    uint16_t Size1;
    uint8_t ID2[4];//same
    uint16_t Size2;
    uint8_t ID3[4];//'02TS'
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t TS_ID[] = {'0', '2', 'T', 'S'};

  BOOST_STATIC_ASSERT(sizeof(Footer) == 16);

  const std::string TS_FOOTER_FORMAT(
    "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID1[4];//'PT3!' or other type
    "?%00xxxxxx"                     // uint16_t Size1;
    "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID2[4];//same
    "?%00xxxxxx"                     // uint16_t Size2;
    "'0'2'T'S"                       // uint8_t ID3[4];//'02TS'
  );

  class ModuleTraits
  {
  public:
    ModuleTraits(const Binary::Data& data, std::size_t footerOffset)
      : FooterOffset(footerOffset)
      , Foot(footerOffset != data.Size() ? safe_ptr_cast<const Footer*>(static_cast<const uint8_t*>(data.Start()) + footerOffset) : 0)
      , FirstSize(Foot ? fromLE(Foot->Size1) : 0)
      , SecondSize(Foot ? fromLE(Foot->Size2) : 0)
    {
    }

    bool Matched() const
    {
      return Foot != 0 && FooterOffset == FirstSize + SecondSize;
    }

    std::size_t NextOffset() const
    {
      if (Foot == 0)
      {
        return FooterOffset;
      }
      const std::size_t totalSize = FirstSize + SecondSize;
      if (totalSize < FooterOffset)
      {
        return FooterOffset - totalSize;
      }
      else
      {
        return FooterOffset + sizeof(*Foot);
      }
    }

    Binary::Container::Ptr GetFirstContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(0, FirstSize);
    }

    Binary::Container::Ptr GetSecondContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(FirstSize, SecondSize);
    }

    Binary::Container::Ptr GetTotalContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(0, FooterOffset + sizeof(*Foot));
    }
  private:
    const std::size_t FooterOffset;
    const Footer* const Foot;
    const std::size_t FirstSize;
    const std::size_t SecondSize;
  };

  class FooterFormat : public Binary::Format
  {
  public:
    typedef boost::shared_ptr<const FooterFormat> Ptr;

    FooterFormat()
      : Delegate(Binary::Format::Create(TS_FOOTER_FORMAT))
    {
    }

    virtual bool Match(const Binary::Data& data) const
    {
      const ModuleTraits traits = GetTraits(data);
      return traits.Matched();
    }

    virtual std::size_t NextMatchOffset(const Binary::Data& data) const
    {
      const ModuleTraits traits = GetTraits(data);
      return traits.NextOffset();
    }

    ModuleTraits GetTraits(const Binary::Data& data) const
    {
      return ModuleTraits(data, Delegate->NextMatchOffset(data));
    }
  private:
    const Binary::Format::Ptr Delegate;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(Binary::Format::Ptr format)
      : Format(format)
    {
    }

    virtual String GetDescription() const
    {
      return Text::TS_PLUGIN_INFO;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual bool Check(const Binary::Container& rawData) const
    {
      return Format->Match(rawData);
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const
    {
      //TODO?
      return Formats::Chiptune::Container::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(FooterFormat::Ptr format)
      : Format(format)
    {
    }

    virtual Module::Holder::Ptr CreateModule(Module::PropertiesBuilder& properties, const Binary::Container& data) const
    {
      const ModuleTraits traits = Format->GetTraits(data);
      if (!traits.Matched())
      {
        return Module::Holder::Ptr();
      }
      try
      {
        const AYM::Chiptune::Ptr tune1 = OpenAYMModule(traits.GetFirstContent(data));
        if (!tune1)
        {
          Dbg("Invalid first module holder");
          return Module::Holder::Ptr();
        }
        const AYM::Chiptune::Ptr tune2 = OpenAYMModule(traits.GetSecondContent(data));
        if (!tune2)
        {
          Dbg("Failed to create second module holder");
          return Module::Holder::Ptr();
        }
        const Binary::Container::Ptr total = traits.GetTotalContent(data);
        properties.SetSource(Formats::Chiptune::CalculatingCrcContainer(total, 0, total->Size()));
        const TurboSound::Chiptune::Ptr chiptune = TurboSound::CreateChiptune(properties.GetResult(), tune1, tune2);
        return TurboSound::CreateHolder(chiptune);
      }
      catch (const Error& e)
      {
        return Module::Holder::Ptr();
      }
    }
  private:
    static AYM::Chiptune::Ptr OpenAYMModule(Binary::Container::Ptr data)
    {
      if (const AYM::Holder::Ptr holder = boost::dynamic_pointer_cast<const AYM::Holder>(Open(*data)))
      {
        return holder->GetChiptune();
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  private:
    const FooterFormat::Ptr Format;
  };
}
}

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'S', 0};
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_TS | CAP_CONV_RAW;

    const Module::TS::FooterFormat::Ptr format = boost::make_shared<Module::TS::FooterFormat>();
    const Formats::Chiptune::Decoder::Ptr decoder = boost::make_shared<Module::TS::Decoder>(format);
    const Module::Factory::Ptr factory = boost::make_shared<Module::TS::Factory>(format);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
