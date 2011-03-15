/*
Abstract:
  Hrum convertors support

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

  const Char HRUM_PLUGIN_ID[] = {'H', 'R', 'U', 'M', '\0'};
  const String HRUM_PLUGIN_VERSION(FromStdString("$Rev$"));

  //////////////////////////////////////////////////////////////////////////
  class HrumPlugin : public ArchivePlugin
  {
  public:
    HrumPlugin()
      : Decoder(Formats::Packed::CreateHrumDecoder())
    {
    }
    
    virtual String Id() const
    {
      return HRUM_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUM_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUM_PLUGIN_VERSION;
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
  void RegisterHrumConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new HrumPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
