/**
* 
* @file
*
* @brief  FSB images support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "decoders.h"
#include "fsb_formats.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <debug/log.h>
//text include
#include <formats/text/archived.h>

namespace Formats
{
namespace Archived
{
  namespace FSB
  {
    const Debug::Stream Dbg("Formats::Archived::FSB");

    const std::string FORMAT(
      "'F'S'B'5"
      "?{20}"
      "01|02|03|04|05|07|0b 000000" //pcm+imaadpcm+mpeg
    );

    class File : public Archived::File
    {
    public:
      File(String name, Binary::Container::Ptr data)
        : Name(std::move(name))
        , Data(std::move(data))
      {
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Data->Size();
      }

      Binary::Container::Ptr GetData() const override
      {
        return Data;
      }
    private:
      const String Name;
      const Binary::Container::Ptr Data;
    };

    typedef std::map<String, Binary::Container::Ptr> NamedDataMap;

    class Container : public Archived::Container
    {
    public:
      Container(Binary::Container::Ptr delegate, NamedDataMap files)
        : Delegate(std::move(delegate))
        , Files(std::move(files))
      {
      }

      //Binary::Container
      const void* Start() const override
      {
        return Delegate->Start();
      }

      std::size_t Size() const override
      {
        return Delegate->Size();
      }

      Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      //Archive::Container
      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& it : Files)
        {
          const File file(it.first, it.second);
          walker.OnFile(file);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const auto it = Files.find(name);
        return it != Files.end()
          ? MakePtr<File>(it->first, it->second)
          : File::Ptr();
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Files.size());
      }
    private:
      const Binary::Container::Ptr Delegate;
      NamedDataMap Files;
    };
    
    //header+single sample header + 1 byte of data
    const std::size_t MIN_SIZE = 60 + 8 + 1;
    
    class DispatchedBuilder : public FormatBuilder
    {
    public:
      void Setup(uint_t samplesCount, uint_t format) override
      {
        switch (format)
        {
        case Fmod::Format::MPEG:
          Delegate = CreateMpegBuilder();
          break;
        case Fmod::Format::PCM8:
        case Fmod::Format::PCM16:
        case Fmod::Format::PCM24:
        case Fmod::Format::PCM32:
        case Fmod::Format::PCMFLOAT:
        case Fmod::Format::IMAADPCM:
          Delegate = CreatePcmBuilder();
          break;
        default:
          Require(false);
          break;
        }
        Delegate->Setup(samplesCount, format);
      }
      
      void StartSample(uint_t idx) override
      {
        Delegate->StartSample(idx);
      }
      
      void SetFrequency(uint_t frequency) override
      {
        Delegate->SetFrequency(frequency);
      }
      
      void SetChannels(uint_t channels) override
      {
        Delegate->SetChannels(channels);
      }
      
      void SetName(String name) override
      {
        Delegate->SetName(std::move(name));
      }
      
      void AddMetaChunk(uint_t type, const Binary::Data& chunk) override
      {
        Delegate->AddMetaChunk(type, chunk);
      }
      
      void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
      {
        Delegate->SetData(samplesCount, std::move(blob));
      }
      
      NamedDataMap CaptureResult() override
      {
        return Delegate->CaptureResult();
      }
    private:
      Ptr Delegate;
    };

    //fill descriptors array and return actual container size
    Container::Ptr ParseArchive(const Binary::Container& data)
    {
      DispatchedBuilder builder;
      if (auto content = Fmod::Parse(data, builder))
      {
        auto files = builder.CaptureResult();
        if (!files.empty())
        {
          return MakePtr<Container>(std::move(content), std::move(files));
        }
      }
      return Container::Ptr();
    }
  }//namespace FSB

  class FSBDecoder : public Decoder
  {
  public:
    FSBDecoder()
      : Format(Binary::CreateFormat(FSB::FORMAT, FSB::MIN_SIZE))
    {
    }

    String GetDescription() const override
    {
      return Text::FSB_DECODER_DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (Format->Match(data))
      {
        return FSB::ParseArchive(data);
      }
      else
      {
        return Container::Ptr();
      }
    }
  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateFSBDecoder()
  {
    return MakePtr<FSBDecoder>();
  }
}//namespace Archived
}//namespace Formats
