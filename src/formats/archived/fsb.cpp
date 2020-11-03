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
#include "formats/archived/fsb_formats.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <formats/archived.h>
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

    const StringView FORMAT(
      "'F'S'B'5"
      "?{20}"
      "01-05|07|0b|0d|0f 000000" //pcm+imaadpcm+mpeg+at9+vorbis
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

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr delegate, NamedDataMap files)
        : BaseContainer(std::move(delegate))
        , Files(std::move(files))
      {
      }

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
        case Fmod::Format::VORBIS:
          Delegate = CreateOggVorbisBuilder();
          break;
        case Fmod::Format::AT9:
          Delegate = CreateAtrac9Builder();
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
      
      void AddMetaChunk(uint_t type, Binary::View chunk) override
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
