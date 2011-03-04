/*
Abstract:
  ESV convertors support

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

  const Char ESV_PLUGIN_ID[] = {'E', 'S', 'V', '\0'};
  const String ESV_PLUGIN_VERSION(FromStdString("$Rev$"));

  class ESVPlugin : public ArchivePlugin
  {
  public:
    ESVPlugin()
      : Decoder(Formats::Packed::CreateESVCruncherDecoder())
    {
    }

    virtual String Id() const
    {
      return ESV_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::ESV_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return ESV_PLUGIN_VERSION;
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
  };
}

namespace ZXTune
{
  void RegisterESVConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new ESVPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
