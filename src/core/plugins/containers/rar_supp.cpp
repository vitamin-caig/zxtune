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
#include <list>
#include <numeric>
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'R', 'A', 'R', '\0'};
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

  class ChainDecoder
  {
  public:
    typedef boost::shared_ptr<const ChainDecoder> Ptr;

    ChainDecoder()
      : Delegate(Formats::Packed::CreateRarDecoder())
    {
    }

    Binary::Container::Ptr Decode(const Binary::Container& data, const String& name) const
    {
      LastDecoded = name;
      return Delegate->Decode(data);
    }

    String GetLastDecoded() const
    {
      return LastDecoded;
    }
  private:
    const Formats::Packed::Decoder::Ptr Delegate;
    mutable String LastDecoded;
  };

  class RarContainerFile : public Container::File
  {
  public:
    typedef boost::shared_ptr<RarContainerFile> Ptr;

    RarContainerFile(ChainDecoder::Ptr decoder, Binary::Container::Ptr data, const String& name, std::size_t size, Container::File::Ptr parent)
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

    virtual Binary::Container::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", Name);
      const String parentName = Parent->GetName();
      if (!parentName.empty() && Decoder->GetLastDecoded() != parentName)
      {
        Log::Debug(THIS_MODULE, " Decompressing parent '%1%'", parentName);
        Parent->GetData();
      }
      return Decoder->Decode(*Data, Name);
    }

    std::size_t GetSize() const
    {
      return Size;
    }
  private:
    const ChainDecoder::Ptr Decoder;
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
    RarFilesIterator(ChainDecoder::Ptr decoder, Binary::Container::Ptr data)
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

    RarContainerFile::Ptr GetFile() const
    {
      assert(IsValid());
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
      assert(!IsEof());
      Previous = IsValid()
        ? GetFile()
        : StubContainerFile::Create();
      Current.reset();
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
    const ChainDecoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    RarBlocksIterator Blocks;
    mutable RarContainerFile::Ptr Current;
    Container::File::Ptr Previous;
  };

  class RarCatalogue : public Container::Catalogue
  {
  public:
    RarCatalogue(std::size_t maxFileSize, Binary::Container::Ptr data)
      : Decoder(boost::make_shared<ChainDecoder>())
      , Data(data)
      , MaxFileSize(maxFileSize)
    {
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

    virtual void ForEachFile(const Container::Catalogue::Callback& cb) const
    {
      FillCache();
      for (FilesList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        cb.OnFile(**it);
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
        const FilesList::const_iterator it = std::find_if(Files.begin(), Files.end(),
          boost::bind(&Container::File::GetName, _1) == name);
        if (it != Files.end())
        {
          return *it;
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
        const RarContainerFile::Ptr fileObject = Iter->GetFile();
        Iter->Next();
        if (fileObject->GetSize() > MaxFileSize)
        {
          Log::Debug(THIS_MODULE, "File is too big (%1% bytes)", fileObject->GetSize());
          continue;
        }
        Files.push_back(fileObject);
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
    const ChainDecoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<RarFilesIterator> Iter;
    typedef std::list<Container::File::Ptr> FilesList;
    mutable FilesList Files;
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
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
