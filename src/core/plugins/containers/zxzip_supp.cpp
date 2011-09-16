/*
Abstract:
  ZXZip convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "trdos_catalogue.h"
#include "trdos_utils.h"
#include "core/plugins/registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'Z', 'X', 'Z', 'I', 'P', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::ZXZIP_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct ZXZipHeader
  {
    //+0x0
    char Name[8];
    //+0x8
    char Type[3];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  String ExtractFileName(const void* data)
  {
    const ZXZipHeader* const header = static_cast<const ZXZipHeader*>(data);
    return TRDos::GetEntryName(header->Name, header->Type);
  }

  Container::Catalogue::Ptr ParseZXZipFile(const Formats::Packed::Decoder& decoder, const Binary::Container& data)
  {
    TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
    const std::size_t archSize = data.Size();
    std::size_t rawOffset = 0;
    for (std::size_t flatOffset = 0; rawOffset < archSize;)
    {
      const Binary::Container::Ptr rawData = data.GetSubcontainer(rawOffset, archSize - rawOffset);
      if (!decoder.Check(*rawData))
      {
        break;
      }
      const Formats::Packed::Container::Ptr fileData = decoder.Decode(*rawData);
      if (!fileData)
      {
        break;
      }
      const String fileName = ExtractFileName(rawData->Data());
      const std::size_t fileSize = fileData->Size();
      const std::size_t usedSize = fileData->PackedSize();
      const TRDos::File::Ptr file = TRDos::File::Create(fileData, fileName, flatOffset, fileSize);
      builder->AddFile(file);
      rawOffset += usedSize;
      flatOffset += fileSize;
    }
    builder->SetUsedSize(rawOffset);
    return builder->GetResult();
  }

  class ZXZipFactory : public ContainerFactory
  {
  public:
    ZXZipFactory()
      : Decoder(Formats::Packed::CreateZXZipDecoder())
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& /*parameters*/, Binary::Container::Ptr data) const
    {
      const Container::Catalogue::Ptr files = ParseZXZipFile(*Decoder, *data);
      return files && files->GetFilesCount()
        ? files
        : Container::Catalogue::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterZXZipContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<ZXZipFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
