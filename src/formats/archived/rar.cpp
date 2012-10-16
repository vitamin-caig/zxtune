/*
Abstract:
  Rar archives support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <debug_log.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <formats/archived.h>
#include <formats/packed/decoders.h>
#include <formats/packed/rar_supp.h>
//std includes
#include <cstring>
#include <list>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text include
#include <formats/text/packed.h>

namespace Rar
{
  const Debug::Stream Dbg("Formats::Archived::Rar");

  using namespace Formats;

  const Packed::Rar::BlockHeader MARKER = 
  {
    fromLE<uint16_t>(0x6152),
    0x72,
    fromLE<uint16_t>(0x1a21),
    fromLE<uint16_t>(0x0007)
  };

  class StubFile : public Archived::File
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

    virtual std::size_t GetSize() const
    {
      return 0;
    }

    static Ptr Create()
    {
      static StubFile stub;
      return Ptr(&stub, NullDeleter<StubFile>());
    }
  };

  class ChainDecoder
  {
  public:
    typedef boost::shared_ptr<const ChainDecoder> Ptr;

    ChainDecoder()
      : Delegate(Packed::CreateRarDecoder())
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

  class File : public Archived::File
  {
  public:
    File(ChainDecoder::Ptr decoder, Binary::Container::Ptr data, const String& name, std::size_t size, Archived::File::Ptr parent)
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

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Dbg("Decompressing '%1%'", Name);
      const String parentName = Parent->GetName();
      if (!parentName.empty() && Decoder->GetLastDecoded() != parentName)
      {
        Dbg(" Decompressing parent '%1%'", parentName);
        Parent->GetData();
      }
      return Decoder->Decode(*Data, Name);
    }
  private:
    const ChainDecoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Size;
    const Archived::File::Ptr Parent;
  };

  class BlocksIterator
  {
  public:
    explicit BlocksIterator(const Binary::Container& data)
      : Container(data)
      , Limit(data.Size())
      , Offset(0)
    {
      assert(0 == std::memcmp(Container.GetField<Packed::Rar::BlockHeader>(Offset), &MARKER, sizeof(MARKER)));
    }

    bool IsEof() const
    {
      const uint64_t curBlockSize = GetBlockSize();
      return !curBlockSize || uint64_t(Offset) + curBlockSize > uint64_t(Limit);
    }

    const Packed::Rar::ArchiveBlockHeader* GetArchiveHeader() const
    {
      assert(!IsEof());
      const Packed::Rar::ArchiveBlockHeader& block = *Container.GetField<Packed::Rar::ArchiveBlockHeader>(Offset);
      return !block.IsExtended() && Packed::Rar::ArchiveBlockHeader::TYPE == block.Type
        ? &block
        : 0;
    }

    const Packed::Rar::FileBlockHeader* GetFileHeader() const
    {
      assert(!IsEof());
      if (const Packed::Rar::FileBlockHeader* const block = Container.GetField<Packed::Rar::FileBlockHeader>(Offset))
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
      if (const Packed::Rar::BlockHeader* const block = Container.GetField<Packed::Rar::BlockHeader>(Offset))
      {
        uint64_t res = fromLE(block->Size);
        if (block->IsExtended())
        {
          if (const Packed::Rar::ExtendedBlockHeader* const extBlock = Container.GetField<Packed::Rar::ExtendedBlockHeader>(Offset))
          {
            res += fromLE(extBlock->AdditionalSize);
            //Even if big files are not supported, we should properly skip them in stream
            if (Packed::Rar::FileBlockHeader::TYPE == extBlock->Type && 
                Packed::Rar::FileBlockHeader::FLAG_BIG_FILE & fromLE(extBlock->Flags))
            {
              if (const Packed::Rar::BigFileBlockHeader* const bigFile = Container.GetField<Packed::Rar::BigFileBlockHeader>(Offset))
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
      if (const Packed::Rar::BlockHeader* block = Container.GetField<Packed::Rar::BlockHeader>(Offset))
      {
        //finish block
        if (block->Type == 0x7b && 7 == fromLE(block->Size))
        {
          Offset += sizeof(*block);
        }
      }
    }
  private:
    const Binary::TypedContainer Container;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class FileIterator
  {
  public:
    FileIterator(ChainDecoder::Ptr decoder, const Binary::Container& data)
      : Decoder(decoder)
      , Data(data)
      , Blocks(data)
      , Previous(StubFile::Create())
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

    Archived::File::Ptr GetFile() const
    {
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      if (file.IsSupported() && !Current)
      {
        const std::size_t offset = Blocks.GetOffset();
        const std::size_t size = Blocks.GetBlockSize();
        const Binary::Container::Ptr fileBlock = Data.GetSubcontainer(offset, size);
        const Archived::File::Ptr prev = (Previous && file.IsSolid()) ? Previous : StubFile::Create();
        Current = boost::make_shared<File>(Decoder, fileBlock, GetName(), fromLE(file.UnpackedSize), prev);
      }
      return Current;
    }

    void Next()
    {
      assert(!IsEof());
      Previous = GetFile();
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
    const Binary::Container& Data;
    BlocksIterator Blocks;
    mutable Archived::File::Ptr Current;
    Archived::File::Ptr Previous;
  };

  class Container : public Archived::Container
  {
  public:
    Container(Binary::Container::Ptr data, uint_t filesCount)
      : Decoder(boost::make_shared<ChainDecoder>())
      , Delegate(data)
      , FilesCount(filesCount)
    {
      Dbg("Found %1% files. Size is %2%", filesCount, Delegate->Size());
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
      FillCache();
      for (FilesList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        walker.OnFile(**it);
      }
    }

    virtual Archived::File::Ptr FindFile(const String& name) const
    {
      if (Archived::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    virtual uint_t CountFiles() const
    {
      return FilesCount;
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Archived::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesList::const_iterator it = std::find_if(Files.begin(), Files.end(),
          boost::bind(&Archived::File::GetName, _1) == name);
        if (it != Files.end())
        {
          return *it;
        }
      }
      return Archived::File::Ptr();
    }

    Archived::File::Ptr FindNonCachedFile(const String& name) const
    {
      CreateIterator();
      while (!Iter->IsEof())
      {
        const String fileName = Iter->GetName();
        if (!Iter->IsValid())
        {
          Dbg("Invalid file '%1%'", fileName);
          Iter->Next();
          continue;
        }
        Dbg("Found file '%1%'", fileName);
        const Archived::File::Ptr fileObject = Iter->GetFile();
        Files.push_back(fileObject);
        Iter->Next();
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Archived::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new FileIterator(Decoder, *Delegate));
      }
    }
  private:
    const ChainDecoder::Ptr Decoder;
    const Binary::Container::Ptr Delegate;
    const uint_t FilesCount;
    mutable std::auto_ptr<FileIterator> Iter;
    typedef std::list<Archived::File::Ptr> FilesList;
    mutable FilesList Files;
  };

  const std::string FORMAT(
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
}

namespace Formats
{
  namespace Archived
  {
    class RarDecoder : public Decoder
    {
    public:
      RarDecoder()
        : Format(Binary::Format::Create(Rar::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::RAR_DECODER_DESCRIPTION;
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

        uint_t filesCount = 0;
        Rar::BlocksIterator iter(data);
        for (; !iter.IsEof(); iter.Next())
        {
          if (const Formats::Packed::Rar::FileBlockHeader* file = iter.GetFileHeader())
          {
            filesCount += file->IsSupported();
          }
        }
        if (const std::size_t totalSize = iter.GetOffset())
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, totalSize);
          return boost::make_shared<Rar::Container>(archive, filesCount);
        }
        else
        {
          return Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateRarDecoder()
    {
      return boost::make_shared<RarDecoder>();
    }
  }
}

