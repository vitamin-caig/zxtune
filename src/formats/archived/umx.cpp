/**
 *
 * @file
 *
 * @brief  UMX containers support
 * @note   http://wiki.beyondunreal.com/Legacy:Package_File_Format
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/decoders.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <strings/casing.h>
#include <strings/map.h>
// std includes
#include <array>

namespace Formats::Archived
{
  namespace UMX
  {
    const Debug::Stream Dbg("Formats::Archived::UMX");

    const Char DESCRIPTION[] = "UMX (Unreal Music eXperience)";
    const auto FORMAT =
        "c1832a9e"     // signature
        "? 00"         // version
        "??"           // license mode
        "????"         // package flags
        "??0000 ????"  // names
        "??0000 ????"  // exports
        "??0000 ????"  // imports
        ""_sv;

    using SignatureType = std::array<uint8_t, 4>;

    const SignatureType SIGNATURE = {{0xc1, 0x83, 0x2a, 0x9e}};

    struct TableDescription
    {
      le_uint32_t Count;
      le_uint32_t Offset;
    };

    struct RawHeader
    {
      SignatureType Signature;
      le_uint16_t PackageVersion;
      le_uint16_t LicenseMode;
      le_uint32_t PackageFlags;
      TableDescription Names;
      TableDescription Exports;
      TableDescription Imports;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 36, "Invalid layout");

    struct Index
    {
      int_t Value = 0;

      Index() = default;
    };

    struct ClassName
    {
      const String Name;

      explicit ClassName(StringView name)
        : Name(Strings::ToLowerAscii(name))
      {}

      bool IsMusic() const
      {
        return Name == "music" || Name == "sound";
      }
    };

    struct Property
    {
      const String Name;

      explicit Property(StringView name)
        : Name(Strings::ToLowerAscii(name))
      {}

      bool IsLimiter() const
      {
        return Name == "none";
      }
    };

    struct NameEntry
    {
      String Name;
      uint32_t Flags = 0;

      NameEntry() = default;
    };

    struct ExportEntry
    {
      Index Class;
      Index Super;
      uint32_t Group = 0;
      Index ObjectName;
      uint32_t ObjectFlags = 0;
      Index SerialSize;
      Index SerialOffset;

      ExportEntry() = default;
    };

    struct ImportEntry
    {
      Index ClassPackage;
      Index ClassName;
      uint32_t Package = 0;
      Index ObjectName;

      ImportEntry() = default;
    };

    class InputStream
    {
    public:
      InputStream(uint_t version, const Binary::Container& data)
        : Version(version)
        , Data(data)
        , Start(static_cast<const uint8_t*>(Data.Start()))
        , Cursor(Start)
        , Limit(Start + Data.Size())
        , MaxUsedSize()
      {}

      // primitives
      template<class T>
      void Read(T& res)
      {
        Require(Cursor + sizeof(T) <= Limit);
        res = ReadLE<T>(Cursor);
        Cursor += sizeof(T);
      }

      void Read(Index& res)
      {
        int_t result = 0;
        bool negative = false;
        for (uint_t i = 0; i < 5; ++i)
        {
          const uint8_t data = ReadByte();
          if (i == 0)
          {
            negative = 0 != (data & 0x80);
            result = data & 0x3f;
            if (0 == (data & 0x40))
            {
              break;
            }
          }
          else if (i == 4)
          {
            result |= int_t(data & 0x1f) << 27;
          }
          else
          {
            result |= int_t(data & 0x7f) << (7 * i - 1);
            if (0 == (data & 0x80))
            {
              break;
            }
          }
        }
        res.Value = negative ? -result : +result;
      }

      void Read(String& res)
      {
        const uint8_t* const limit = Version >= 64 ? Cursor + ReadByte() : Limit;
        Require(limit <= Limit);
        const uint8_t* const strEnd = std::find(Cursor, limit, 0);
        Require(strEnd != Limit);
        res.assign(Cursor, strEnd);
        Cursor = strEnd + 1;
      }

      // complex types
      void Read(NameEntry& res)
      {
        Read(res.Name);
        Read(res.Flags);
      }

      void Read(ExportEntry& res)
      {
        Read(res.Class);
        Read(res.Super);
        Read(res.Group);
        Read(res.ObjectName);
        Read(res.ObjectFlags);
        Read(res.SerialSize);
        if (res.SerialSize.Value > 0)
        {
          Read(res.SerialOffset);
        }
      }

      void Read(ImportEntry& res)
      {
        Read(res.ClassPackage);
        Read(res.ClassName);
        Read(res.Package);
        Read(res.ObjectName);
      }

      void Read(Property& res)
      {
        Require(!res.IsLimiter());
        // TODO: implement
        Require(false);
      }

      Binary::Container::Ptr ReadContainer(std::size_t size)
      {
        Require(Cursor + size <= Limit);
        Cursor += size;
        return Data.GetSubcontainer(Cursor - size - Start, size);
      }

      Binary::Container::Ptr ReadRestContainer()
      {
        return ReadContainer(Limit - Cursor);
      }

      void Seek(std::size_t pos)
      {
        Require(Start + pos < Limit);
        MaxUsedSize = GetMaxUsedSize();
        Cursor = Start + pos;
      }

      std::size_t GetMaxUsedSize()
      {
        return std::max<std::size_t>(MaxUsedSize, Cursor - Start);
      }

    private:
      uint8_t ReadByte()
      {
        Require(Cursor != Limit);
        return *Cursor++;
      }

    private:
      const uint_t Version;
      const Binary::Container& Data;
      const uint8_t* const Start;
      const uint8_t* Cursor;
      const uint8_t* const Limit;
      std::size_t MaxUsedSize;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Data(data)
        , Header(*safe_ptr_cast<const RawHeader*>(data.Start()))
        , UsedSize(sizeof(Header))
      {
        Require(Header.Signature == SIGNATURE);
        InputStream stream(Header.PackageVersion, Data);
        ReadNames(stream);
        ReadExports(stream);
        ReadImports(stream);
        UsedSize = stream.GetMaxUsedSize();
      }

      std::size_t GetUsedSize() const
      {
        return UsedSize;
      }

      uint_t GetEntriesCount() const
      {
        return Exports.size();
      }

      String GetEntryName(uint_t idx) const
      {
        const auto& exp = Exports.at(idx);
        return GetName(exp.ObjectName);
      }

      Binary::Container::Ptr GetEntryData(uint_t idx) const
      {
        try
        {
          const ExportEntry& exp = Exports.at(idx);
          const std::size_t offset = exp.SerialOffset.Value;
          const std::size_t size = exp.SerialSize.Value;
          const Binary::Container::Ptr entryData = Data.GetSubcontainer(offset, size);
          Require(entryData.get() != nullptr);
          InputStream stream(Header.PackageVersion, *entryData);
          ReadProperties(stream);
          const ClassName& cls = GetClass(exp.Class);
          Dbg("Entry[{}] data at {} size={} class={}", idx, offset, size, cls.Name);
          const Binary::Container::Ptr result = cls.IsMusic() ? ExtractMusicData(stream) : stream.ReadRestContainer();
          UsedSize = std::max(UsedSize, offset + stream.GetMaxUsedSize());
          return result;
        }
        catch (const std::exception&)
        {
          return Binary::Container::Ptr();
        }
      }

    private:
      void ReadNames(InputStream& stream)
      {
        stream.Seek(Header.Names.Offset);
        const uint_t count = Header.Names.Count;
        Names.resize(count);
        for (uint_t idx = 0; idx != count; ++idx)
        {
          NameEntry& entry = Names[idx];
          stream.Read(entry);
          Dbg("Names[{}] = '{}'", idx, entry.Name);
        }
      }

      void ReadExports(InputStream& stream)
      {
        stream.Seek(Header.Exports.Offset);
        const uint_t count = Header.Exports.Count;
        Exports.resize(count);
        for (uint_t idx = 0; idx != count; ++idx)
        {
          ExportEntry& entry = Exports[idx];
          stream.Read(entry);
          Dbg("Exports[{}] = {class={} name={} super={} size={}}", idx, entry.Class.Value, entry.ObjectName.Value,
              entry.Super.Value, entry.SerialSize.Value);
        }
      }

      void ReadImports(InputStream& stream)
      {
        stream.Seek(Header.Imports.Offset);
        const uint_t count = Header.Imports.Count;
        Imports.resize(count);
        for (uint_t idx = 0; idx != count; ++idx)
        {
          ImportEntry& entry = Imports[idx];
          stream.Read(entry);
          Dbg("Imports[{}] = {pkg={} class={} name={}}", idx, entry.ClassPackage.Value, entry.ClassName.Value,
              entry.ObjectName.Value);
        }
      }

      const String& GetName(Index idx) const
      {
        return Names.at(idx.Value).Name;
      }

      ClassName GetClass(Index idx) const
      {
        if (idx.Value == 0)
        {
          return ClassName("(null)");
        }
        else if (idx.Value < 0)
        {
          const ImportEntry& entry = Imports.at(-idx.Value - 1);
          return ClassName(GetName(entry.ObjectName));
        }
        else
        {
          const ExportEntry& entry = Exports.at(idx.Value - 1);
          return ClassName(GetName(entry.ObjectName));
        }
      }

      void ReadProperties(InputStream& stream) const
      {
        for (;;)
        {
          Index nameIdx;
          stream.Read(nameIdx);
          Property prop(GetName(nameIdx));
          if (prop.IsLimiter())
          {
            break;
          }
          stream.Read(prop);
        }
      }

      Binary::Container::Ptr ExtractMusicData(InputStream& stream) const
      {
        Index format;
        format.Value = -1000;
        const uint_t version = Header.PackageVersion;
        if (version >= 120)
        {
          uint32_t flags, aux;
          stream.Read(format);
          stream.Read(flags);
          stream.Read(aux);
        }
        else if (version >= 100)
        {
          uint32_t flags, aux;
          stream.Read(flags);
          stream.Read(format);
          stream.Read(aux);
        }
        else if (version >= 62)
        {
          uint32_t flags;
          stream.Read(format);
          stream.Read(flags);
        }
        else
        {
          stream.Read(format);
        }
        Index size;
        stream.Read(size);
        Dbg("Extract music data from container (version={} format={} size={})", version, format.Value, size.Value);
        return stream.ReadContainer(size.Value);
      }

    private:
      const Binary::Container& Data;
      const RawHeader Header;
      mutable std::size_t UsedSize;
      std::vector<NameEntry> Names;
      std::vector<ExportEntry> Exports;
      std::vector<ImportEntry> Imports;
    };

    class File : public Archived::File
    {
    public:
      File(StringView name, Binary::Container::Ptr data)
        : Name(name.to_string())
        , Data(std::move(data))
      {}

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

    using NamedDataMap = Strings::ValueMap<Binary::Container::Ptr>;

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr delegate, NamedDataMap files)
        : BaseContainer(std::move(delegate))
        , Files(std::move(files))
      {}

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& it : Files)
        {
          const File file(it.first, it.second);
          walker.OnFile(file);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        if (auto data = Files.Get(name))
        {
          return MakePtr<File>(name, std::move(data));
        }
        else
        {
          return {};
        }
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Files.size());
      }

    private:
      NamedDataMap Files;
    };
  }  // namespace UMX

  class UMXDecoder : public Decoder
  {
  public:
    UMXDecoder()
      : Format(Binary::CreateFormat(UMX::FORMAT))
    {}

    String GetDescription() const override
    {
      return UMX::DESCRIPTION;
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
      const UMX::Format format(data);
      UMX::NamedDataMap datas;
      for (uint_t idx = 0, lim = format.GetEntriesCount(); idx != lim; ++idx)
      {
        if (auto data = format.GetEntryData(idx))
        {
          const auto& name = format.GetEntryName(idx);
          datas.emplace(name, std::move(data));
        }
      }
      if (!datas.empty())
      {
        auto archive = data.GetSubcontainer(0, format.GetUsedSize());
        return MakePtr<UMX::Container>(std::move(archive), std::move(datas));
      }
      UMX::Dbg("No files found");
      return {};
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateUMXDecoder()
  {
    return MakePtr<UMXDecoder>();
  }
}  // namespace Formats::Archived
