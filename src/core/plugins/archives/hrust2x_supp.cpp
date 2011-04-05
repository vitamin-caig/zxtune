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
//boost includes
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char HRUST2X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '2', '\0'};
  const String HRUST2X_PLUGIN_VERSION(FromStdString("$Rev$"));

  class Hrust2xPlugin : public ArchivePlugin
                      , public boost::enable_shared_from_this<Hrust2xPlugin>
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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModulesInArchive(shared_from_this(), *Decoder, inputData, callback);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*parameters*/,
                                   DataLocation::Ptr inputData,
                                   const String& pathToOpen) const
    {
      return OpenDataFromArchive(shared_from_this(), *Decoder, inputData, pathToOpen);
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
  void RegisterHrust2xConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new Hrust2xPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
