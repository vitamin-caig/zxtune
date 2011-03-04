/*
Abstract:
  CodeCruncher v3 convertors support

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

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
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
  void RegisterCC3Convertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new CC3Plugin());
    enumerator.RegisterPlugin(plugin);
  }
}
