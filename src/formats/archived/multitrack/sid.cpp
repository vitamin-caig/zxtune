/**
* 
* @file
*
* @brief  SID containers support
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <binary/format_factories.h>
#include <formats/archived/decoders.h>
#include <formats/chiptune/emulation/sid.h>
//std includes
#include <sstream>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace MultiSID
{
  class File : public Formats::Archived::File
  {
  public:
    File(const String& name, Binary::Container::Ptr data)
      : Name(name)
      , Data(data)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Data->Size();
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }
  private:
    const String Name;
    const Binary::Container::Ptr Data;
  };

  class Filename
  {
  public:
    Filename(const String& prefix, const String& value)
      : Str(value)
      , Valid(false)
      , Index()
    {
      if (0 == value.compare(0, prefix.size(), prefix))
      {
        std::basic_istringstream<Char> str(value.substr(prefix.size()));
        Valid = !!(str >> Index);
      }
    }

    Filename(const String& prefix, uint_t index)
      : Valid(true)
      , Index(index)
    {
      std::basic_ostringstream<Char> str;
      str << prefix << index;
      Str = str.str();
    }

    bool IsValid() const
    {
      return Valid;
    }

    uint_t GetIndex() const
    {
      return Index;
    }

    String ToString() const
    {
      return Str;
    }
  private:
    String Str;
    bool Valid;
    uint_t Index;
  };

  class Container : public Formats::Archived::Container
  {
  public:
    explicit Container(Binary::Container::Ptr data)
      : Delegate(data)
    {
    }

    //Binary::Container
    virtual const void* Start() const
    {
      return Delegate->Start();
    }

    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Container
    virtual void ExploreFiles(const Formats::Archived::Container::Walker& walker) const
    {
      for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
      {
        const uint_t song = idx + 1;
        const String subPath = Filename(Text::MULTITRACK_FILENAME_PREFIX, song).ToString();
        const Binary::Container::Ptr subData = Formats::Chiptune::SID::FixStartSong(*Delegate, song);
        const File file(subPath, subData);
        walker.OnFile(file);
      }
    }

    virtual Formats::Archived::File::Ptr FindFile(const String& name) const
    {
      const Filename filename(Text::MULTITRACK_FILENAME_PREFIX, name);
      if (!filename.IsValid())
      {
        return Formats::Archived::File::Ptr();
      }
      const uint_t song = filename.GetIndex();
      if (song < 1 || song > CountFiles())
      {
        return Formats::Archived::File::Ptr();
      }
      const Binary::Container::Ptr subData = Formats::Chiptune::SID::FixStartSong(*Delegate, song);
      return boost::make_shared<File>(name, subData);
    }

    virtual uint_t CountFiles() const
    {
      return Formats::Chiptune::SID::GetModulesCount(*Delegate);
    }
  private:
    const Binary::Container::Ptr Delegate;
  };

  const std::string FORMAT =
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
}

namespace Formats
{
  namespace Archived
  {
    class MultiSIDDecoder : public Formats::Archived::Decoder
    {
    public:
      MultiSIDDecoder()
        : Format(Binary::CreateMatchOnlyFormat(MultiSID::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SID_ARCHIVE_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const uint_t subModules = Formats::Chiptune::SID::GetModulesCount(rawData);
        if (subModules < 2)
        {
          return Container::Ptr();
        }
        const Binary::Container::Ptr sidData = rawData.GetSubcontainer(0, rawData.Size());
        return boost::make_shared<MultiSID::Container>(sidData);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateSIDDecoder()
    {
      return boost::make_shared<MultiSIDDecoder>();
    }
  }
}
