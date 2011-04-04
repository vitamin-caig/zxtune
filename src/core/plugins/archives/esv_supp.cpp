/*
Abstract:
  ESV convertors support

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
//boost includes
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ESV_PLUGIN_ID[] = {'E', 'S', 'V', '\0'};
  const String ESV_PLUGIN_VERSION(FromStdString("$Rev$"));

  class ESVPlugin : public ArchivePlugin
                  , public boost::enable_shared_from_this<ESVPlugin>
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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModulesInArchive(shared_from_this(), *Decoder, inputData, callback);
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
  void RegisterESVConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new ESVPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
