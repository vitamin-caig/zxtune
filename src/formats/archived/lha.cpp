/*
Abstract:
  Lha archives support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "lha.h"
//common includes
#include <contract.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <formats/archived.h>
//3rdparty includes
#include <3rdparty/lhasa/lib/lha_decoder.h>
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

  const std::string THIS_MODULE("Formats::Archived::Lha");

  using namespace Formats;

  class MemoryFileI
  {
  public:
    explicit MemoryFileI(const Binary::Container& rawData)
      : Data(static_cast<const uint8_t*>(rawData.Data()))
      , Limit(rawData.Size())
      , Position(0)
    {
    }

    std::size_t Read(void* buf, std::size_t len)
    {
      const std::size_t res = std::min(len, Limit - Position);
      std::memcpy(buf, Data + Position, res);
      Position += res;
      return res;
    }

    std::size_t Skip(std::size_t len)
    {
      if (len + Position >= Limit)
      {
        return 0;
      }
      Position += len;
      return 1;
    }

    std::size_t GetPosition() const
    {
      return Position;
    }
  private:
    const uint8_t* const Data;
    const std::size_t Limit;
    std::size_t Position;
  };

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
      return static_cast<int>(static_cast<MemoryFileI*>(handle)->Read(buf, len));
    }

    static int Skip(void* handle, size_t bytes)
    {
      return static_cast<int>(static_cast<MemoryFileI*>(handle)->Skip(bytes));
    }
  private:
    MemoryFileI State;
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
      Log::Debug(THIS_MODULE, "Created file '%1%', size=%2%, packed size=%3%, compression=%4%", Name, Size, Data->Size(), Method);
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
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", Name);
      return Archived::Lha::DecodeRawData(*Data, Method, Size);
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
    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual const void* Data() const
    {
      return Delegate->Data();
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

  //raw data processing
  class Decompressor
  {
  public:
    typedef boost::shared_ptr<const Decompressor> Ptr;
    virtual ~Decompressor() {}

    virtual Binary::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const = 0;
  };

  class LHADecompressor : public Decompressor
  {
  public:
    explicit LHADecompressor(LHADecoderType* type)
      : Type(type)
    {
    }

    virtual Binary::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const
    {
      MemoryFileI input(rawData);
      const boost::shared_ptr<LHADecoder> decoder(::lha_decoder_new(Type, &ReadData, &input, outputSize), &::lha_decoder_free);
      std::auto_ptr<Dump> result(new Dump(outputSize));
      const std::size_t decoded = ::lha_decoder_read(decoder.get(), &result->front(), outputSize);
      if (decoded == outputSize)
      {
        return Binary::CreateContainer(result);
      }
      Log::Debug(THIS_MODULE, "Output size mismatch while decoding");
      return Binary::Container::Ptr();
    }
  private:
    static size_t ReadData(void* buf, size_t len, void* data)
    {
      return static_cast<MemoryFileI*>(data)->Read(buf, len);
    }
  private:
    LHADecoderType* const Type;
  };

  //TODO: create own decoders for non-packing modes
  Decompressor::Ptr CreateDecompressor(const std::string& method)
  {
    if (LHADecoderType* type = ::lha_decoder_for_name(const_cast<char*>(method.c_str())))
    {
      return boost::make_shared<LHADecompressor>(type);
    }
    return Decompressor::Ptr();
  }
}

namespace Formats
{
  namespace Archived
  {
    namespace Lha
    {
      Binary::Container::Ptr DecodeRawData(const Binary::Container& input, const std::string& method, std::size_t outputSize)
      {
        if (const ::Lha::Decompressor::Ptr decompressor = ::Lha::CreateDecompressor(method))
        {
          return decompressor->Decode(input, outputSize);
        }
        return Binary::Container::Ptr();
      }
    }

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
        if (!Format->Match(data.Data(), data.Size()))
        {
          return Container::Ptr();
        }
        ::Lha::FilesIterator iter(data);
        std::list<Archived::File::Ptr> files;
        for (; iter.IsValid(); iter.Next())
        {
          if (!iter.IsDir())
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

