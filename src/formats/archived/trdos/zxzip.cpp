/**
 *
 * @file
 *
 * @brief  ZXZIP archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/trdos/catalogue.h"
#include "formats/archived/trdos/utils.h"
#include "formats/packed/decoders.h"

#include "formats/archived/decoder.h"

#include "make_ptr.h"

namespace Formats::Archived
{
  namespace ZXZip
  {
    struct ZXZipHeader
    {
      //+0x0
      char Name[8];
      //+0x8
      char Type[3];
    };

    static_assert(sizeof(ZXZipHeader) * alignof(ZXZipHeader) == 11, "Invalid layout");

    String ExtractFileName(const void* data)
    {
      const auto* const header = safe_ptr_cast<const ZXZipHeader*>(data);
      return TRDos::GetEntryName(header->Name, header->Type);
    }

    Container::Ptr ParseArchive(const Packed::Decoder& decoder, const Binary::Container& data)
    {
      const auto builder = TRDos::CatalogueBuilder::CreateGeneric();
      const std::size_t archSize = data.Size();
      std::size_t rawOffset = 0;
      for (std::size_t flatOffset = 0; rawOffset < archSize;)
      {
        const auto rawData = data.GetSubcontainer(rawOffset, archSize - rawOffset);
        const auto fileData = decoder.Decode(*rawData);
        if (!fileData)
        {
          break;
        }
        const String fileName = ExtractFileName(rawData->Start());
        const std::size_t fileSize = fileData->Size();
        const std::size_t usedSize = fileData->PackedSize();
        auto file = TRDos::File::Create(fileData, fileName, flatOffset, fileSize);
        builder->AddFile(std::move(file));
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
        return {};
      }
    }

    class Decoder : public Archived::Decoder
    {
    public:
      StringView GetDescription() const override
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
          return {};
        }

        const auto files = ParseArchive(*FileDecoder, data);
        return files && files->CountFiles() ? files : Container::Ptr();
      }

    private:
      const Packed::Decoder::Ptr FileDecoder = Packed::CreateZXZipDecoder();
    };
  }  // namespace ZXZip

  Decoder::Ptr CreateZXZipDecoder()
  {
    return MakePtr<ZXZip::Decoder>();
  }
}  // namespace Formats::Archived
