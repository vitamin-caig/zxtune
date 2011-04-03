/*
Abstract:
  CodeCruncher v3 convertors support

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

  const Char CC3_PLUGIN_ID[] = {'C', 'C', '3', '\0'};
  const String CC3_PLUGIN_VERSION(FromStdString("$Rev$"));

  class CC3Plugin : public ArchivePlugin
  {
  public:
    CC3Plugin()
      : Decoder(Formats::Packed::CreateCodeCruncher3Decoder())
    {
    }

    virtual String Id() const
    {
      return CC3_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::CC3_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return CC3_PLUGIN_VERSION;
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
  void RegisterCC3Convertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new CC3Plugin());
    registrator.RegisterPlugin(plugin);
  }
}
