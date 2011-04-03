/*
Abstract:
  Trush convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "extraction_result.h"
#include <core/plugins/registrator.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
#include <io/container.h>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char TRUSH_PLUGIN_ID[] = {'T', 'R', 'U', 'S', 'H', '\0'};
  const String TRUSH_PLUGIN_VERSION(FromStdString("$Rev$"));

  class TRUSHPlugin : public ArchivePlugin
  {
  public:
    TRUSHPlugin()
      : Decoder(Formats::Packed::CreateTRUSHDecoder())
    {
    }

    virtual String Id() const
    {
      return TRUSH_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::TRUSH_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return TRUSH_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return Decoder->Check(inputData.Data(), inputData.Size());
    }

    virtual ArchiveExtractionResult::Ptr ExtractSubdata(IO::DataContainer::Ptr input) const
    {
      return ExtractDataFromArchive(*Decoder, input);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterTRUSHConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new TRUSHPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
