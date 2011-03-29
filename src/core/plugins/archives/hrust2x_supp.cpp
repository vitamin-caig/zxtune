/*
Abstract:
  Hrust 2.x convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "extraction_result.h"
#include <core/plugins/registrator.h>
//common includes
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
#include <io/container.h>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char HRUST2X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '2', '\0'};
  const String HRUST2X_PLUGIN_VERSION(FromStdString("$Rev$"));

  class Hrust2xPlugin : public ArchivePlugin
  {
  public:
    Hrust2xPlugin()
      : Decoder(Formats::Packed::CreateHrust2Decoder())
    {
    }

    virtual String Id() const
    {
      return HRUST2X_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUST2X_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUST2X_PLUGIN_VERSION;
    }
    
    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return Decoder->Check(inputData.Data(), inputData.Size());
    }

    virtual ArchiveExtractionResult::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      IO::DataContainer::Ptr input) const
    {
      return ExtractDataFromArchive(*Decoder, input);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterHrust2xConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new Hrust2xPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
