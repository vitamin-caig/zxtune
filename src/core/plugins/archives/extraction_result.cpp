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
#include "core/src/callback.h"
#include "core/src/core.h"
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  class ArchiveDetectionResultImpl : public DetectionResult
  {
  public:
    ArchiveDetectionResultImpl(DataFormat::Ptr format, IO::DataContainer::Ptr data)
      : Format(format)
      , RawData(data)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return 0;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return Format->Search(RawData->Data(), RawData->Size());
    }
  private:
    const DataFormat::Ptr Format;
    const IO::DataContainer::Ptr RawData;
  };

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
  DetectionResult::Ptr DetectModulesInArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder, 
    DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const IO::DataContainer::Ptr rawData = inputData->GetData();
    std::size_t packedSize = 0;
    std::auto_ptr<Dump> res = decoder.Decode(rawData->Data(), rawData->Size(), packedSize);
    if (res.get())
    {
      const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
      const IO::DataContainer::Ptr subData = IO::CreateDataContainer(res);
      const String subPath = Text::ARCHIVE_PLUGIN_PREFIX + plugin->Id();
      const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(inputData, plugin, subData, subPath);
      ZXTune::Module::Detect(subLocation, noProgressCallback);
      return DetectionResult::Create(packedSize, 0);
    }
    const DataFormat::Ptr format = decoder.GetFormat();
    return boost::make_shared<ArchiveDetectionResultImpl>(format, rawData);
  }

  ArchiveExtractionResult::Ptr ExtractDataFromArchive(const Formats::Packed::Decoder& decoder, IO::DataContainer::Ptr data)
  {
    return boost::make_shared<ArchiveExtractionResultImpl>(decoder, data);
  }
}
