/*
Abstract:
  ST3 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_conversion.h"
#include "soundtracker.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/archives/archive_supp_common.h"
#include "core/plugins/players/creation_result.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/packed/decoders.h>
//boost includes
#include <boost/make_shared.hpp>

namespace ST3
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', '3', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::GetSupportedAYMFormatConvertors();

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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      const ::SoundTracker::ModuleData::RWPtr modData = boost::make_shared< ::SoundTracker::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::SoundTracker::Builder> dataBuilder = ::SoundTracker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SoundTracker::ParseVersion3(*data, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const Module::AYM::Chiptune::Ptr chiptune = ::SoundTracker::CreateChiptune(modData, properties);
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ST3
{
  const Char IDC[] = {'C', 'O', 'M', 'P', 'I', 'L', 'E', 'D', 'S', 'T', '3', 0};
  const uint_t CCAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterST3Support(PluginsRegistrator& registrator)
  {
    //modules with players
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCompiledST3Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ST3::IDC, ST3::CCAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSoundTracker3Decoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ST3::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ST3::ID, decoder->GetDescription(), ST3::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
