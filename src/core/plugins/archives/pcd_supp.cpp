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
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char PCD_PLUGIN_ID[] = {'P', 'C', 'D', '\0'};
  const String PCD_PLUGIN_VERSION(FromStdString("$Rev$"));

  class PCDPlugin : public ArchivePlugin
  {
  public:
    PCDPlugin()
      : Decoder61(Formats::Packed::CreatePowerfullCodeDecreaser61Decoder())
      , Decoder62(Formats::Packed::CreatePowerfullCodeDecreaser62Decoder())
    {
    }
    
    virtual String Id() const
    {
      return PCD_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PCD_PLUGIN_INFO;
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
      const void* const data = inputData.Data();
      const std::size_t size = inputData.Size();
      return Decoder61->Check(data, size) || Decoder62->Check(data, size);
    }

    virtual ArchiveExtractionResult::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      IO::DataContainer::Ptr input) const
    {
      const void* const data = input->Data();
      const std::size_t size = input->Size();
      if (Decoder61->Check(data, size))
      {
        return ExtractDataFromArchive(*Decoder61, input);
      }
      return ExtractDataFromArchive(*Decoder62, input);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder61;
    const Formats::Packed::Decoder::Ptr Decoder62;
  };
}

namespace ZXTune
{
  void RegisterPCDConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new PCDPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
