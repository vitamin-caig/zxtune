/**
* 
* @file
*
* @brief  SID containers support
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <formats/archived/decoders.h>
#include <formats/chiptune/emulation/sid.h>
#include <strings/prefixed_index.h>
//std includes
#include <utility>
//text includes
#include <formats/text/archived.h>

namespace Formats
{
namespace Archived
{
  namespace MultiSID
  {
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

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      explicit Container(Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
      {
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
        {
          const uint_t song = idx + 1;
          const String subPath = Strings::PrefixedIndex(Text::MULTITRACK_FILENAME_PREFIX, song).ToString();
          const Binary::Container::Ptr subData = Formats::Chiptune::SID::FixStartSong(*Delegate, song);
          const File file(subPath, subData);
          walker.OnFile(file);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const Strings::PrefixedIndex filename(Text::MULTITRACK_FILENAME_PREFIX, name);
        if (!filename.IsValid())
        {
          return File::Ptr();
        }
        const uint_t song = filename.GetIndex();
        if (song < 1 || song > CountFiles())
        {
          return File::Ptr();
        }
        const Binary::Container::Ptr subData = Formats::Chiptune::SID::FixStartSong(*Delegate, song);
        return MakePtr<File>(name, subData);
      }

      uint_t CountFiles() const override
      {
        return Formats::Chiptune::SID::GetModulesCount(*Delegate);
      }
    };

    const StringView FORMAT =
        "'R|'P 'S'I'D" //signature
        "00 01-03"     //BE version
        "00 76|7c"     //BE data offset
        "??"           //BE load address
        "??"           //BE init address
        "??"           //BE play address
        "00|01 ?"      //BE songs count 1-256
        "??"           //BE start song
        "????"         //BE speed flag
     ;
  }//namespace MultiSID

  class MultiSIDDecoder : public Decoder
  {
  public:
    MultiSIDDecoder()
      : Format(Binary::CreateMatchOnlyFormat(MultiSID::FORMAT))
    {
    }

    String GetDescription() const override
    {
      return Text::SID_ARCHIVE_DECODER_DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const uint_t subModules = Formats::Chiptune::SID::GetModulesCount(rawData);
      if (subModules < 2)
      {
        return Container::Ptr();
      }
      const Binary::Container::Ptr sidData = rawData.GetSubcontainer(0, rawData.Size());
      return MakePtr<MultiSID::Container>(sidData);
    }
  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateSIDDecoder()
  {
    return MakePtr<MultiSIDDecoder>();
  }
}//namespace Archived
}//namespace Formats
