/*
Abstract:
  ZXZip convertors support

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

  const Char ZXZIP_PLUGIN_ID[] = {'Z', 'X', 'Z', 'I', 'P', '\0'};
  const String ZXZIP_PLUGIN_VERSION(FromStdString("$Rev$"));

  //////////////////////////////////////////////////////////////////////////
  class ZXZipPlugin : public ArchivePlugin
                     , public boost::enable_shared_from_this<ZXZipPlugin>
  {
  public:
    ZXZipPlugin()
      : Decoder(Formats::Packed::CreateZXZipDecoder())
    {
    }
    
    virtual String Id() const
    {
      return ZXZIP_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::ZXZIP_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return ZXZIP_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModulesInArchive(shared_from_this(), *Decoder, inputData, callback);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*parameters*/,
                                   DataLocation::Ptr inputData,
                                   const DataPath& pathToOpen) const
    {
      return OpenDataFromArchive(shared_from_this(), *Decoder, inputData, pathToOpen);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterZXZipConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new ZXZipPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
