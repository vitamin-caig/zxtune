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
//common includes
#include <make_ptr.h>
//std includes
#include <sstream>
//text includes
#include <formats/text/archived.h>

namespace Formats
{
namespace Archived
{
  namespace MultitrackArchives
  {
    class File : public Archived::File
    {
    public:
      File(const String& name, Binary::Container::Ptr data)
        : Name(name)
        , Data(data)
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

    class Container : public Archived::Container
    {
    public:
      explicit Container(Formats::Multitrack::Container::Ptr data)
        : Delegate(data)
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

      //Container
      void ExploreFiles(const Container::Walker& walker) const override
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

      File::Ptr FindFile(const String& name) const override
      {
        const Filename filename(Text::MULTITRACK_FILENAME_PREFIX, name);
        if (!filename.IsValid())
        {
          return File::Ptr();
        }
        const uint_t song = filename.GetIndex();
        if (song < 1 || song > CountFiles())
        {
          return File::Ptr();
        }
        const uint_t idx = song - 1;
        const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
        return MakePtr<File>(name, subData);
      }

      uint_t CountFiles() const override
      {
        return Delegate->TracksCount();
      }
    private:
      const Formats::Multitrack::Container::Ptr Delegate;
    };

    class Decoder : public Archived::Decoder
    {
    public:
      Decoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
        : Description(description)
        , Delegate(delegate)
      {
      }

      String GetDescription() const override
      {
        return Description;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Delegate->GetFormat();
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (const Formats::Multitrack::Container::Ptr data = Delegate->Decode(rawData))
        {
          if (data->TracksCount() > 1)
          {
            return MakePtr<Container>(data);
          }
        }
        return Container::Ptr();
      }
    private:
      const String Description;
      const Formats::Multitrack::Decoder::Ptr Delegate;
    };
  }//namespace MultitrackArchives

  Decoder::Ptr CreateMultitrackArchiveDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
  {
    return MakePtr<MultitrackArchives::Decoder>(description, delegate);
  }
}//namespace Archived
}//namespace Formats
