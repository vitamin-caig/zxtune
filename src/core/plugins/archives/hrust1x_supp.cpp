/*
Abstract:
  Hrust 1.x convertors support

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
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 047CB563

namespace
{
  using namespace ZXTune;

  const Char HRUST1X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '1', '\0'};
  const String HRUST1X_PLUGIN_VERSION(FromStdString("$Rev$"));

  class Hrust1xPlugin : public ArchivePlugin
  {
  public:
    Hrust1xPlugin()
      : Decoder(Formats::Packed::CreateHrust1Decoder())
    {
    }

    virtual String Id() const
    {
      return HRUST1X_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUST1X_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUST1X_PLUGIN_VERSION;
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
  void RegisterHrust1xConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new Hrust1xPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
