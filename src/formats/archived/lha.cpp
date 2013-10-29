/**
* 
* @file
*
* @brief  LHA archives support
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
//library includes
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/lha_supp.h>
#include <formats/packed/pack_utils.h>
//3rdparty includes
#include <3rdparty/lhasa/lib/public/lhasa.h>
//std includes
#include <cstring>
#include <list>
#include <map>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text include
#include <formats/text/archived.h>

namespace Lha
{
  const std::string FORMAT(
    "??"        //size+sum/size/size len
    "'-('l|'p)('z|'h|'m)('s|'d|'0-'7)'-" //method, see lha_decoder.c for all available
    "????"      //packed size
    "????"      //original size
    "????"      //time
    "%00xxxxxx" //attr/0x20
    "00-03"     //level
  );

  const Debug::Stream Dbg("Formats::Archived::Lha");

  using namespace Formats;

  class InputStreamWrapper
  {
  public:
    explicit InputStreamWrapper(const Binary::Container& data)
      : State(data)
    {
      Vtable.read = &Read;
      Vtable.skip = &Skip;
      Vtable.close = 0;
      Stream = boost::shared_ptr<LHAInputStream>(::lha_input_stream_new(&Vtable, &State), &::lha_input_stream_free);
    }

    LHAInputStream* GetStream() const
    {
      return Stream.get();
    }

    std::size_t GetPosition() const
    {
      return State.GetPosition();
    }
  private:
    static int Read(void* handle, void* buf, size_t len)
    {
      return static_cast<int>(static_cast<Binary::InputStream*>(handle)->Read(buf, len));
    }

    static int Skip(void* handle, size_t bytes)
    {
      Binary::InputStream* const stream = static_cast<Binary::InputStream*>(handle);
      const std::size_t rest = stream->GetRestSize();
      if (rest >= bytes)
      {
        stream->ReadData(bytes);//skip
        return 1;
      }
      return 0;
    }
  private:
    Binary::InputStream State;
    LHAInputStreamType Vtable;
    boost::shared_ptr<LHAInputStream> Stream;
  };

  String GetFullPath(const LHAFileHeader& header)
  {
    const String filename(FromStdString(header.filename));
    return header.path
      ? FromStdString(header.path) + filename
      : filename;
  }

  class File : public Archived::File
  {
  public:
    File(const Binary::Container& archive, const LHAFileHeader& header, std::size_t position)
      : Data(archive.GetSubcontainer(position, header.compressed_length))
      , Name(GetFullPath(header))
      , Size(header.length)
      , Method(FromStdString(header.compress_method))
    {
      Dbg("Created file '%1%', size=%2%, packed size=%3%, compression=%4%", Name, Size, Data->Size(), Method);
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Dbg("Decompressing '%1%'", Name);
      return Packed::Lha::DecodeRawData(*Data, Method, Size);
    }
  private:
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Size;
    const std::string Method;
  };

  class FilesIterator
  {
  public:
    explicit FilesIterator(const Binary::Container& data)
      : Data(data)
      , Input(data)
      , Reader(::lha_reader_new(Input.GetStream()), &::lha_reader_free)
      , Current()
      , Position()
    {
      Next();
    }

    bool IsValid() const
    {
      return Current != 0;
    }

    bool IsDir() const
    {
      static const char DIR_TYPE[] = "-lhd-";
      Require(Current != 0);
      return 0 == std::strcmp(DIR_TYPE, Current->compress_method);
    }

    bool IsEmpty() const
    {
      Require(Current != 0);
      return 0 == Current->compressed_length;
    }

    Archived::File::Ptr GetFile() const
    {
      Require(Current != 0);
      return boost::make_shared<File>(Data, *Current, Position);
    }

    std::size_t GetOffset() const
    {
      return Input.GetPosition();
    }

    void Next()
    {
      Current = ::lha_reader_next_file(Reader.get());
      Position = Input.GetPosition();
    }
  private:
    const Binary::Container& Data;
    const InputStreamWrapper Input;
    const boost::shared_ptr<LHAReader> Reader;
    LHAFileHeader* Current;
    std::size_t Position;
  };

  class Container : public Archived::Container
  {
  public:
    template<class It>
    Container(Binary::Container::Ptr data, It begin, It end)
      : Delegate(data)
    {
      for (It it = begin; it != end; ++it)
      {
        const Archived::File::Ptr file = *it;
        Files.insert(FilesMap::value_type(file->GetName(), file));
      }
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

    //Archive::Container
    virtual void ExploreFiles(const Archived::Container::Walker& walker) const
    {
      for (FilesMap::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        walker.OnFile(*it->second);
      }
    }

    virtual Archived::File::Ptr FindFile(const String& name) const
    {
      const FilesMap::const_iterator it = Files.find(name);
      return it != Files.end()
        ? it->second
        : Archived::File::Ptr();
    }

    virtual uint_t CountFiles() const
    {
      return static_cast<uint_t>(Files.size());
    }
  private:
    const Binary::Container::Ptr Delegate;
    typedef std::map<String, Archived::File::Ptr> FilesMap;
    FilesMap Files;
  };
}

namespace Formats
{
  namespace Archived
  {
    class LhaDecoder : public Decoder
    {
    public:
      LhaDecoder()
        : Format(Binary::Format::Create(::Lha::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::LHA_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        if (!Format->Match(data))
        {
          return Container::Ptr();
        }
        ::Lha::FilesIterator iter(data);
        std::list<Archived::File::Ptr> files;
        for (; iter.IsValid(); iter.Next())
        {
          if (!iter.IsDir() && !iter.IsEmpty())
          {
            const Archived::File::Ptr file = iter.GetFile();
            files.push_back(file);
          }
        }
        if (const std::size_t totalSize = iter.GetOffset())
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, totalSize);
          return boost::make_shared< ::Lha::Container>(archive, files.begin(), files.end());
        }
        else
        {
          return Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateLhaDecoder()
    {
      return boost::make_shared<LhaDecoder>();
    }
  }
}

