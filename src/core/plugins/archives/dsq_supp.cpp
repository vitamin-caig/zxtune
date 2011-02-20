/*
Abstract:
  DSQ convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
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
      return Decoder->Check(inputData.Data(), inputData.Size());
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*commonParams*/,
      const IO::DataContainer& data, ModuleRegion& region) const
    {
      std::auto_ptr<Dump> res = Decoder->Decode(data.Data(), data.Size(), region.Size);
      if (res.get())
      {
        region.Offset = 0;
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterDSQConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new DSQPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
