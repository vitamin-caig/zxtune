/**
* 
* @file
*
* @brief  NSF containers support
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <binary/format_factories.h>
#include <formats/archived/decoders.h>
#include <formats/chiptune/emulation/nsf.h>
//std includes
#include <sstream>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace MultiNSF
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
    explicit Container(Formats::Chiptune::PolyTrackContainer::Ptr data)
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
        const String subPath = Filename(Text::NSF_FILENAME_PREFIX, song).ToString();
        const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
        const File file(subPath, subData);
        walker.OnFile(file);
      }
    }

    virtual Formats::Archived::File::Ptr FindFile(const String& name) const
    {
      const Filename filename(Text::NSF_FILENAME_PREFIX, name);
      if (!filename.IsValid())
      {
        return Formats::Archived::File::Ptr();
      }
      const uint_t song = filename.GetIndex();
      if (song < 1 || song > CountFiles())
      {
        return Formats::Archived::File::Ptr();
      }
      const uint_t idx = song - 1;
      const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
      return boost::make_shared<File>(name, subData);
    }

    virtual uint_t CountFiles() const
    {
      return Delegate->TracksCount();
    }
  private:
    const Formats::Chiptune::PolyTrackContainer::Ptr Delegate;
  };

  const std::string FORMAT =
      "'N'E'S'M"
      "1a"
      "?"     //version
      "02-ff" //2 songs minimum
      "01-ff"
      //gme supports nfs load/init address starting from 0x8000 or zero
      "(? 00|80-ff){2}"
   ;

    const std::size_t MIN_SIZE = 256;
}

namespace Formats
{
  namespace Archived
  {
    class MultiNSFDecoder : public Formats::Archived::Decoder
    {
    public:
      MultiNSFDecoder()
        : Format(Binary::CreateMatchOnlyFormat(MultiNSF::FORMAT, MultiNSF::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::NSF_ARCHIVE_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (const Formats::Chiptune::PolyTrackContainer::Ptr data = Formats::Chiptune::NSF::Parse(rawData))
        {
          if (data->TracksCount() > 1)
          {
            return boost::make_shared<MultiNSF::Container>(data);
          }
        }
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateNSFDecoder()
    {
      return boost::make_shared<MultiNSFDecoder>();
    }
  }
}
