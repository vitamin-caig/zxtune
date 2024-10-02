/**
 *
 * @file
 *
 * @brief  LHA archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/lha_supp.h>
#include <formats/packed/pack_utils.h>
#include <strings/encoding.h>
#include <strings/map.h>
// 3rdparty includes
#include <3rdparty/lhasa/lib/public/lhasa.h>
// std includes
#include <cstring>
#include <list>
#include <numeric>

namespace Formats::Archived
{
  namespace Lha
  {
    const Debug::Stream Dbg("Formats::Archived::Lha");

    const Char DESCRIPTION[] = "LHA (LHArc)";
    const auto FORMAT =
        "??"                                  // size+sum/size/size len
        "'-('l|'p)('z|'h|'m)('s|'d|'0-'7)'-"  // method, see lha_decoder.c for all available
        "????"                                // packed size
        "????"                                // original size
        "????"                                // time
        "%00xxxxxx"                           // attr/0x20
        "00-03"                               // level
        ""sv;

    class InputStreamWrapper
    {
    public:
      explicit InputStreamWrapper(const Binary::Container& data)
        : State(data)
      {
        Vtable.read = &Read;
        Vtable.skip = &Skip;
        Vtable.close = nullptr;
        Stream = std::shared_ptr<LHAInputStream>(::lha_input_stream_new(&Vtable, &State), &::lha_input_stream_free);
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
        auto* const stream = static_cast<Binary::InputStream*>(handle);
        const std::size_t rest = stream->GetRestSize();
        if (rest >= bytes)
        {
          stream->Skip(bytes);
          return 1;
        }
        return 0;
      }

    private:
      Binary::InputStream State;
      LHAInputStreamType Vtable;
      std::shared_ptr<LHAInputStream> Stream;
    };

    String GetFullPath(const LHAFileHeader& header)
    {
      String fullPath;
      if (header.path)
      {
        fullPath = header.path;
      }
      fullPath += header.filename;
      return Strings::ToAutoUtf8(fullPath);
    }

    class File : public Archived::File
    {
    public:
      File(const Binary::Container& archive, const LHAFileHeader& header, std::size_t position)
        : Data(archive.GetSubcontainer(position, header.compressed_length))
        , Name(GetFullPath(header))
        , Size(header.length)
        , Method(header.compress_method)
      {
        Dbg("Created file '{}', size={}, packed size={}, compression={}", Name, Size, Data->Size(), Method);
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Size;
      }

      Binary::Container::Ptr GetData() const override
      {
        Dbg("Decompressing '{}'", Name);
        return Packed::Lha::DecodeRawData(*Data, Method, Size);
      }

    private:
      const Binary::Container::Ptr Data;
      const String Name;
      const std::size_t Size;
      const String Method;
    };

    class FilesIterator
    {
    public:
      explicit FilesIterator(const Binary::Container& data)
        : Data(data)
        , Input(data)
        , Reader(::lha_reader_new(Input.GetStream()), &::lha_reader_free)
      {
        Next();
      }

      bool IsValid() const
      {
        return Current != nullptr;
      }

      bool IsDir() const
      {
        static const char DIR_TYPE[] = "-lhd-";
        Require(Current != nullptr);
        return 0 == std::strcmp(DIR_TYPE, Current->compress_method);
      }

      bool IsEmpty() const
      {
        Require(Current != nullptr);
        return 0 == Current->compressed_length || Position + Current->compressed_length > Data.Size();
      }

      File::Ptr GetFile() const
      {
        Require(Current != nullptr);
        return MakePtr<File>(Data, *Current, Position);
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
      const std::shared_ptr<LHAReader> Reader;
      LHAFileHeader* Current = nullptr;
      std::size_t Position = 0;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      template<class It>
      Container(Binary::Container::Ptr&& data, It begin, It end)
        : BaseContainer(std::move(data))
      {
        for (It it = begin; it != end; ++it)
        {
          const auto file = *it;
          Files.emplace(file->GetName(), file);
        }
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& file : Files)
        {
          walker.OnFile(*file.second);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        return Files.Get(name);
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Files.size());
      }

    private:
      Strings::ValueMap<File::Ptr> Files;
    };
  }  // namespace Lha

  class LhaDecoder : public Decoder
  {
  public:
    LhaDecoder()
      : Format(Binary::CreateFormat(Lha::FORMAT))
    {}

    String GetDescription() const override
    {
      return Lha::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (!Format->Match(data))
      {
        return {};
      }
      Lha::FilesIterator iter(data);
      std::list<File::Ptr> files;
      for (; iter.IsValid(); iter.Next())
      {
        if (!iter.IsDir() && !iter.IsEmpty())
        {
          const File::Ptr file = iter.GetFile();
          files.push_back(file);
        }
      }
      if (const std::size_t totalSize = iter.GetOffset())
      {
        auto archive = data.GetSubcontainer(0, totalSize);
        return MakePtr<Lha::Container>(std::move(archive), files.begin(), files.end());
      }
      else
      {
        return {};
      }
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateLhaDecoder()
  {
    return MakePtr<LhaDecoder>();
  }
}  // namespace Formats::Archived
