/*
Abstract:
  MsPack convertors support

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

  const Char MSP_PLUGIN_ID[] = {'M', 'S', 'P', '\0'};
  const String MSP_PLUGIN_VERSION(FromStdString("$Rev$"));

  class MSPPlugin : public ArchivePlugin
                  , public boost::enable_shared_from_this<MSPPlugin>
  {
  public:
    MSPPlugin()
      : Decoder(Formats::Packed::CreateMSPackDecoder())
    {
    }

    virtual String Id() const
    {
      return MSP_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::MSP_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return MSP_PLUGIN_VERSION;
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
  void RegisterMSPConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new MSPPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
