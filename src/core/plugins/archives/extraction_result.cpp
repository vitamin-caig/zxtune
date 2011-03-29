/*
Abstract:
  Archive extraction result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "extraction_result.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class ArchiveExtractionResultImpl : public ArchiveExtractionResult
  {
  public:
    ArchiveExtractionResultImpl(const Formats::Packed::Decoder& decoder, IO::DataContainer::Ptr data)
      : Decoder(decoder)
      , RawData(data)
      , PackedSize(0)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      TryToExtract();
      return PackedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      const DataFormat::Ptr format = Decoder.GetFormat();
      return format->Search(RawData->Data(), RawData->Size());
    }

    virtual IO::DataContainer::Ptr GetExtractedData() const
    {
      TryToExtract();
      return ExtractedData;
    }
  private:
    void TryToExtract() const
    {
      if (PackedSize)
      {
        return;
      }
      std::auto_ptr<Dump> res = Decoder.Decode(RawData->Data(), RawData->Size(), PackedSize);
      if (res.get())
      {
        ExtractedData = IO::CreateDataContainer(res);
      }
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const IO::DataContainer::Ptr RawData;
    mutable std::size_t PackedSize;
    mutable IO::DataContainer::Ptr ExtractedData;
  };
}

namespace ZXTune
{
  ArchiveExtractionResult::Ptr ExtractDataFromArchive(const Formats::Packed::Decoder& decoder, IO::DataContainer::Ptr data)
  {
    return boost::make_shared<ArchiveExtractionResultImpl>(decoder, data);
  }
}
