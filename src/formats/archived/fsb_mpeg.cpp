/**
* 
* @file
*
* @brief  FSB MPEG images support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/archived/fsb_formats.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
#include <xrange.h>

namespace Formats::Archived::FSB
{
  namespace Mpeg
  {
    class Builder : public FormatBuilder
    {
    public:
      void Setup(uint_t samplesCount, uint_t format) override
      {
        Require(format == Fmod::Format::MPEG);
        Names.resize(samplesCount);
        Blobs.resize(samplesCount);
      }
      
      void StartSample(uint_t idx) override
      {
        CurSample = idx;
      }
      
      void SetFrequency(uint_t /*frequency*/) override
      {
      }
      
      void SetChannels(uint_t /*channels*/) override
      {
      }
      
      void SetName(String name) override
      {
        Names[CurSample] = std::move(name);
      }
      
      void AddMetaChunk(uint_t /*type*/, Binary::View /*chunk*/) override
      {
      }
      
      void SetData(uint_t /*samplesCount*/, Binary::Container::Ptr blob) override
      {
        Blobs[CurSample] = std::move(blob);
      }
      
      NamedDataMap CaptureResult() override
      {
        NamedDataMap result;
        for (uint_t idx : xrange(Names.size()))
        {
          if (auto& blob = Blobs[idx])
          {
            result.emplace(std::move(Names[idx]), std::move(blob));
          }
        }
        return result;
      }
    private:
      uint_t CurSample = 0;
      std::vector<String> Names;
      std::vector<Binary::Container::Ptr> Blobs;
    };
  }

  FormatBuilder::Ptr CreateMpegBuilder()
  {
    return MakePtr<Mpeg::Builder>();
  }
}
