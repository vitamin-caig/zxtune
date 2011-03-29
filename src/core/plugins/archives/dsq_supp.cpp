/*
Abstract:
  DSQ convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
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

  const Char DSQ_PLUGIN_ID[] = {'D', 'S', 'Q', '\0'};
  const String DSQ_PLUGIN_VERSION(FromStdString("$Rev$"));

  class DSQPlugin : public ArchivePlugin
  {
  public:
    DSQPlugin()
      : Decoder(Formats::Packed::CreateDataSquieezerDecoder())
      , Format(Decoder->GetFormat())
    {
    }

    virtual String Id() const
    {
      return DSQ_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::DSQ_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return DSQ_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return Format->Match(inputData.Data(), inputData.Size());
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*commonParams*/,
      const IO::DataContainer& data, std::size_t& usedSize) const
    {
      std::auto_ptr<Dump> res = Decoder->Decode(data.Data(), data.Size(), usedSize);
      if (res.get())
      {
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterDSQConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new DSQPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
