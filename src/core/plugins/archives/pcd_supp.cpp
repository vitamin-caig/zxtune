/*
Abstract:
  PCD convertors support

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

  const Char PCD61_PLUGIN_ID[] = {'P', 'C', 'D', '6', '1', '\0'};
  const Char PCD62_PLUGIN_ID[] = {'P', 'C', 'D', '6', '2', '\0'};
  const String PCD_PLUGIN_VERSION(FromStdString("$Rev$"));

  class PCD61Plugin : public ArchivePlugin
                    , public boost::enable_shared_from_this<PCD61Plugin>
  {
  public:
    PCD61Plugin()
      : Decoder(Formats::Packed::CreatePowerfullCodeDecreaser61Decoder())
    {
    }
    
    virtual String Id() const
    {
      return PCD61_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PCD61_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return PCD_PLUGIN_VERSION;
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

  class PCD62Plugin : public ArchivePlugin
                    , public boost::enable_shared_from_this<PCD62Plugin>
  {
  public:
    PCD62Plugin()
      : Decoder(Formats::Packed::CreatePowerfullCodeDecreaser62Decoder())
    {
    }
    
    virtual String Id() const
    {
      return PCD62_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PCD62_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return PCD_PLUGIN_VERSION;
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
  void RegisterPCDConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin61(new PCD61Plugin());
    const ArchivePlugin::Ptr plugin62(new PCD62Plugin());
    registrator.RegisterPlugin(plugin61);
    registrator.RegisterPlugin(plugin62);
  }
}
