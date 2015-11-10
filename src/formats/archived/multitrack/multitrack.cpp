/**
* 
* @file
*
* @brief  Multitrack archives support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multitrack.h"
//std includes
#include <sstream>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace MultitrackArchives
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
    explicit Container(Formats::Multitrack::Container::Ptr data)
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
        const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
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
      const uint_t idx = song - 1;
      const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
      return boost::make_shared<File>(name, subData);
    }

    virtual uint_t CountFiles() const
    {
      return Delegate->TracksCount();
    }
  private:
    const Formats::Multitrack::Container::Ptr Delegate;
  };

  class Decoder : public Formats::Archived::Decoder
  {
  public:
    Decoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
      : Description(description)
      , Delegate(delegate)
    {
    }

    virtual String GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Delegate->GetFormat();
    }

    virtual Formats::Archived::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      if (const Formats::Multitrack::Container::Ptr data = Delegate->Decode(rawData))
      {
        if (data->TracksCount() > 1)
        {
          return boost::make_shared<Container>(data);
        }
      }
      return Formats::Archived::Container::Ptr();
    }
  private:
    const String Description;
    const Formats::Multitrack::Decoder::Ptr Delegate;
  };
}

namespace Formats
{
  namespace Archived
  {
    Formats::Archived::Decoder::Ptr CreateMultitrackArchiveDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
    {
      return boost::make_shared<MultitrackArchives::Decoder>(description, delegate);
    }
  }
}
