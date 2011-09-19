/*
Abstract:
  Rar convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/plugins/registrator.h"
#include "core/src/path.h"
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
#include <formats/packed/rar_supp.h>
//std includes
#include <numeric>
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'R', 'A', 'R', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::RAR_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string THIS_MODULE("Core::RARSupp");

  const Formats::Packed::Rar::BlockHeader MARKER = 
  {
    fromLE(0x6152),
    0x72,
    fromLE(0x1a21),
    fromLE(0x0007)
  };

  class StubContainerFile : public Container::File
  {
  public:
    virtual String GetName() const
    {
      return String();
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return 0;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Binary::Container::Ptr();
    }

    static Ptr Create()
    {
      static StubContainerFile stub;
      return Ptr(&stub, NullDeleter<StubContainerFile>());
    }
  };

  class RarContainerFile : public Container::File
  {
  public:
    RarContainerFile(Formats::Packed::Decoder::Ptr decoder, Binary::Container::Ptr data, const String& name, std::size_t size, Container::File::Ptr parent)
      : Decoder(decoder)
      , Data(data)
      , Name(name)
      , Size(size)
      , Parent(parent)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Parent->GetData();
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", Name);
      return Decoder->Decode(*Data);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Size;
    const Container::File::Ptr Parent;
  };

  class RarBlocksIterator
  {
  public:
    explicit RarBlocksIterator(Binary::Container::Ptr data)
      : Data(data)
      , Limit(Data->Size())
      , Offset(0)
    {
      assert(0 == std::memcmp(Get<Formats::Packed::Rar::BlockHeader>(), &MARKER, sizeof(MARKER)));
    }

    bool IsEof() const
    {
      const uint64_t curBlockSize = GetBlockSize();
      return !curBlockSize || uint64_t(Offset) + curBlockSize > uint64_t(Limit);
    }

    const Formats::Packed::Rar::BlockHeader& GetCurrentBlock() const
    {
      return *Get<Formats::Packed::Rar::BlockHeader>();
    }

    const Formats::Packed::Rar::ArchiveBlockHeader* GetArchiveHeader() const
    {
      assert(!IsEof());
      const Formats::Packed::Rar::ArchiveBlockHeader& block = *Get<Formats::Packed::Rar::ArchiveBlockHeader>();
      return !block.IsExtended() && Formats::Packed::Rar::ArchiveBlockHeader::TYPE == block.Type
        ? &block
        : 0;
    }

    const Formats::Packed::Rar::FileBlockHeader* GetFileHeader() const
    {
      assert(!IsEof());
      if (const Formats::Packed::Rar::FileBlockHeader* const block = Get<Formats::Packed::Rar::FileBlockHeader>())
      {
        return block->IsValid()
          ? block
          : 0;
      }
      return 0;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }

    uint64_t GetBlockSize() const
    {
      if (const Formats::Packed::Rar::BlockHeader* const block = Get<Formats::Packed::Rar::BlockHeader>())
      {
        uint64_t res = fromLE(block->Size);
        if (block->IsExtended())
        {
          if (const Formats::Packed::Rar::ExtendedBlockHeader* const extBlock = Get<Formats::Packed::Rar::ExtendedBlockHeader>())
          {
            res += fromLE(extBlock->AdditionalSize);
            //Even if big files are not supported, we should properly skip them in stream
            if (Formats::Packed::Rar::FileBlockHeader::TYPE == extBlock->Type && 
                Formats::Packed::Rar::FileBlockHeader::FLAG_BIG_FILE & fromLE(extBlock->Flags))
            {
              if (const Formats::Packed::Rar::BigFileBlockHeader* const bigFile = Get<Formats::Packed::Rar::BigFileBlockHeader>())
              {
                res += uint64_t(fromLE(bigFile->PackedSizeHi)) << (8 * sizeof(uint32_t));
              }
              else
              {
                return sizeof(*bigFile);
              }
            }
          }
          else
          {
            return sizeof(*extBlock);
          }
        }
        return res;
      }
      else
      {
        return sizeof(*block);
      }
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetBlockSize();
      if (const Formats::Packed::Rar::BlockHeader* block = Get<Formats::Packed::Rar::BlockHeader>())
      {
        //finish block
        if (block->Type == 0x7b && 7 == fromLE(block->Size))
        {
          Offset += sizeof(*block);
        }
      }
    }
  private:
    template<class T>
    const T* Get() const
    {
      if (Limit - Offset >= sizeof(T))
      {
        return safe_ptr_cast<const T*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
      }
      else
      {
        return 0;
      }
    }
  private:
    const Binary::Container::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class RarFilesIterator
  {
  public:
    RarFilesIterator(Formats::Packed::Decoder::Ptr decoder, Binary::Container::Ptr data)
      : Decoder(decoder)
      , Data(data)
      , Blocks(data)
      , Previous(StubContainerFile::Create())
    {
      SkipNonFileBlocks();
    }

    bool IsEof() const
    {
      return Blocks.IsEof();
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      return file.IsSupported();
    }

    String GetName() const
    {
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      String name = file.GetName();
      std::replace(name.begin(), name.end(), '\\', '/');
      return name;
    }

    Container::File::Ptr GetFile() const
    {
      if (!IsValid())
      {
        return StubContainerFile::Create();
      }
      if (!Current)
      {
        const std::size_t offset = Blocks.GetOffset();
        const std::size_t size = Blocks.GetBlockSize();
        const Binary::Container::Ptr fileBlock = Data->GetSubcontainer(offset, size);
        const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
        const Container::File::Ptr prev = file.IsSolid() ? Previous : StubContainerFile::Create();
        Current = boost::make_shared<RarContainerFile>(Decoder, fileBlock, GetName(), fromLE(file.UnpackedSize), prev);
      }
      return Current;
    }

    void Next()
    {
      Previous = GetFile();
      Current.reset();
      assert(!IsEof());
      Blocks.Next();
      SkipNonFileBlocks();
    }

    std::size_t GetOffset() const
    {
      return Blocks.GetOffset();
    }
  private:
    void SkipNonFileBlocks()
    {
      while (!Blocks.IsEof() && !Blocks.GetFileHeader())
      {
        Blocks.Next();
      }
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    RarBlocksIterator Blocks;
    mutable Container::File::Ptr Current;
    Container::File::Ptr Previous;
  };

  template<class It>
  class FilesIterator : public Container::Catalogue::Iterator
  {
  public:
    FilesIterator(It begin, It limit)
      : Current(begin)
      , Limit(limit)
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual Container::File::Ptr Get() const
    {
      assert(IsValid());
      return Current->second;
    }

    virtual void Next()
    {
      assert(IsValid());
      ++Current;
    }
  private:
    It Current;
    const It Limit;
  };

  class RarCatalogue : public Container::Catalogue
  {
  public:
    RarCatalogue(std::size_t maxFileSize, Binary::Container::Ptr data)
      : Decoder(Formats::Packed::CreateRarDecoder())
      , Data(data)
      , MaxFileSize(maxFileSize)
    {
    }

    virtual Iterator::Ptr GetFiles() const
    {
      FillCache();
      return Iterator::Ptr(new FilesIterator<FilesMap::const_iterator>(Files.begin(), Files.end()));
    }

    virtual uint_t GetFilesCount() const
    {
      FillCache();
      return Files.size();
    }

    virtual Container::File::Ptr FindFile(const DataPath& path) const
    {
      const String inPath = path.AsString();
      const String firstComponent = path.GetFirstComponent();
      if (inPath == firstComponent)
      {
        return FindFile(firstComponent);
      }
      Log::Debug(THIS_MODULE, "Opening '%1%'", inPath);
      DataPath::Ptr resolved = CreateDataPath(firstComponent);
      for (;;)
      {
        const String filename = resolved->AsString();
        Log::Debug(THIS_MODULE, "Trying '%1%'", filename);
        if (Container::File::Ptr file = FindFile(filename))
        {
          Log::Debug(THIS_MODULE, "Found");
          return file;
        }
        if (filename == inPath)
        {
          return Container::File::Ptr();
        }
        const DataPath::Ptr unresolved = SubstractDataPath(path, *resolved);
        resolved = CreateMergedDataPath(resolved, unresolved->GetFirstComponent());
      }
    }

    virtual std::size_t GetSize() const
    {
      FillCache();
      assert(Iter->IsEof());
      return Iter->GetOffset();
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Container::File::Ptr FindFile(const String& name) const
    {
      if (Container::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    Container::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesMap::const_iterator it = Files.find(name);
        if (it != Files.end())
        {
          return it->second;
        }
      }
      return Container::File::Ptr();
    }

    Container::File::Ptr FindNonCachedFile(const String& name) const
    {
      CreateIterator();
      while (!Iter->IsEof())
      {
        const String fileName = Iter->GetName();
        if (!Iter->IsValid())
        {
          Log::Debug(THIS_MODULE, "Invalid file '%1%'", fileName);
          Iter->Next();
          continue;
        }
        Log::Debug(THIS_MODULE, "Found file '%1%'", fileName);
        const Container::File::Ptr fileObject = Iter->GetFile();
        Iter->Next();
        if (fileObject->GetSize() > MaxFileSize)
        {
          Log::Debug(THIS_MODULE, "File is too big (%1% bytes)", fileObject->GetSize());
          continue;
        }
        Files.insert(FilesMap::value_type(fileName, fileObject));
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Container::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new RarFilesIterator(Decoder, Data));
      }
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<RarFilesIterator> Iter;
    typedef std::map<String, Container::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };

  const std::string RAR_FORMAT(
    //file marker
    "5261"       //uint16_t CRC;   "Ra"
    "72"         //uint8_t Type;   "r"
    "211a"       //uint16_t Flags; "!ESC^"
    "0700"       //uint16_t Size;  "BELL^,0"
    //archive header
    "??"         //uint16_t CRC;
    "73"         //uint8_t Type;
    "??"         //uint16_t Flags;
    "0d00"       //uint16_t Size
  );

  class RarFactory : public ContainerFactory
  {
  public:
    RarFactory()
      : Format(Binary::Format::Create(RAR_FORMAT))
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, Binary::Container::Ptr data) const
    {
      if (!Format->Match(data->Data(), data->Size()))
      {
        return Container::Catalogue::Ptr();
      }
      //TODO: own parameters
      Parameters::IntType maxFileSize = Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT;
      parameters.FindIntValue(Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB, maxFileSize);
      maxFileSize *= 1 << 20;
      if (maxFileSize > std::numeric_limits<std::size_t>::max())
      {
        maxFileSize = std::numeric_limits<std::size_t>::max();
      }
      return boost::make_shared<RarCatalogue>(static_cast<std::size_t>(maxFileSize), data);
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterRarContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<RarFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
