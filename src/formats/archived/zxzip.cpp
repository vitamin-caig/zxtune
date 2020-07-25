/**
* 
* @file
*
* @brief  ZXZIP archives support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/archived/trdos_catalogue.h"
#include "formats/archived/trdos_utils.h"
//common includes
#include <make_ptr.h>
//library includes
#include <formats/packed/decoders.h>

namespace Formats
{
namespace Archived
{
  namespace ZXZip
  {
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

    Container::Ptr ParseArchive(const Packed::Decoder& decoder, const Binary::Container& data)
    {
      const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
      const std::size_t archSize = data.Size();
      std::size_t rawOffset = 0;
      for (std::size_t flatOffset = 0; rawOffset < archSize;)
      {
        const Binary::Container::Ptr rawData = data.GetSubcontainer(rawOffset, archSize - rawOffset);
        const Formats::Packed::Container::Ptr fileData = decoder.Decode(*rawData);
        if (!fileData)
        {
          break;
        }
        const String fileName = ExtractFileName(rawData->Start());
        const std::size_t fileSize = fileData->Size();
        const std::size_t usedSize = fileData->PackedSize();
        const TRDos::File::Ptr file = TRDos::File::Create(fileData, fileName, flatOffset, fileSize);
        builder->AddFile(file);
        rawOffset += usedSize;
        flatOffset += fileSize;
      }
      if (rawOffset)
      {
        builder->SetRawData(data.GetSubcontainer(0, rawOffset));
        return builder->GetResult();
      }
      else
      {
        return Container::Ptr();
      }
    }
  }//namespace ZXZip

  class ZXZipDecoder : public Decoder
  {
  public:
    ZXZipDecoder()
      : FileDecoder(Formats::Packed::CreateZXZipDecoder())
    {
    }

    String GetDescription() const override
    {
      return FileDecoder->GetDescription();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return FileDecoder->GetFormat();
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (!FileDecoder->GetFormat()->Match(data))
      {
        return Container::Ptr();
      }

      const Container::Ptr files = ZXZip::ParseArchive(*FileDecoder, data);
      return files && files->CountFiles()
        ? files
        : Container::Ptr();
    }
  private:
    const Formats::Packed::Decoder::Ptr FileDecoder;
  };

  Decoder::Ptr CreateZXZipDecoder()
  {
    return MakePtr<ZXZipDecoder>();
  }
}//namespace Archived
}//namespace Formats
