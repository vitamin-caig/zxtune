/*
Abstract:
  ZXZip convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core/plugins/registrator.h"
#include "core/plugins/containers/trdos_process.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
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

  TRDos::Catalogue::Ptr ParseZXZipFile(const Formats::Packed::Decoder& decoder, IO::DataContainer::Ptr data)
  {
    TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
    const uint8_t* const archData = static_cast<const uint8_t*>(data->Data());
    const std::size_t archSize = data->Size();
    std::size_t rawOffset = 0;
    for (std::size_t flatOffset = 0; rawOffset < archSize;)
    {
      const uint8_t* const rawData = archData + rawOffset;
      const std::size_t rawSize = archSize - rawOffset;
      if (!decoder.Check(rawData, rawSize))
      {
        break;
      }
      std::size_t usedSize = 0;
      std::auto_ptr<Dump> decoded = decoder.Decode(rawData, rawSize, usedSize);
      if (!decoded.get() || !usedSize)
      {
        break;
      }
      const IO::DataContainer::Ptr fileData = IO::CreateDataContainer(decoded);
      const String fileName = ExtractFileName(rawData);
      const std::size_t fileSize = fileData->Size();
      const TRDos::File::Ptr file = TRDos::File::Create(fileData, fileName, flatOffset, fileSize);
      builder->AddFile(file);
      rawOffset += usedSize;
      flatOffset += fileSize;
    }
    builder->SetUsedSize(rawOffset);
    return builder->GetResult();
  }

  class ZXZipPlugin : public ArchivePlugin
  {
  public:
    ZXZipPlugin()
      : Description(CreatePluginDescription(ID, INFO, VERSION, CAPS))
      , Decoder(Formats::Packed::CreateZXZipDecoder())
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseZXZipFile(*Decoder, rawData))
      {
        if (files->GetFilesCount())
        {
          ProcessEntries(input, callback, Description, *files);
          return DetectionResult::CreateMatched(files->GetUsedSize());
        }
      }
      const DataFormat::Ptr format = Decoder->GetFormat();
      return DetectionResult::CreateUnmatched(format, rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      if (pathComp.empty())
      {
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseZXZipFile(*Decoder, inData))
      {
        if (const TRDos::File::Ptr fileToOpen = files->FindFile(pathComp))
        {
          if (const IO::DataContainer::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description, pathComp); 
          }
        }
      }
      return DataLocation::Ptr();
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
    const ArchivePlugin::Ptr plugin(new ZXZipPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
