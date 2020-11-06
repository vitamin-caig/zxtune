/**
* 
* @file
*
* @brief  AY/EMUL support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/emulation/ay.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
#include <range_checker.h>
//library includes
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <debug/log.h>
#include <formats/chiptune.h>
#include <math/numeric.h>
#include <strings/optimize.h>
//std includes
#include <array>
#include <cstring>
#include <list>
#include <type_traits>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace AY
  {
    const Debug::Stream Dbg("Formats::Chiptune::AY");

    const uint8_t SIGNATURE[] = {'Z', 'X', 'A', 'Y'};
  #ifdef USE_PRAGMA_PACK
  #pragma pack(push,1)
  #endif
    PACK_PRE struct Header
    {
      uint8_t Signature[4];//ZXAY
      uint8_t Type[4];
      uint8_t FileVersion;
      uint8_t PlayerVersion;
      int16_t SpecialPlayerOffset;
      int16_t AuthorOffset;
      int16_t MiscOffset;
      uint8_t LastModuleIndex;
      uint8_t FirstModuleIndex;
      int16_t DescriptionsOffset;
    } PACK_POST;

    PACK_PRE struct ModuleDescription
    {
      int16_t TitleOffset;
      int16_t DataOffset;
    } PACK_POST;

    namespace EMUL
    {
      const uint8_t SIGNATURE[] = {'E', 'M', 'U', 'L'};

      PACK_PRE struct ModuleData
      {
        uint8_t AmigaChannelsMapping[4];
        uint16_t TotalLength;
        uint16_t FadeLength;
        uint16_t RegValue;
        int16_t PointersOffset;
        int16_t BlocksOffset;
      } PACK_POST;

      PACK_PRE struct ModulePointers
      {
        uint16_t SP;
        uint16_t InitAddr;
        uint16_t PlayAddr;
      } PACK_POST;

      PACK_PRE struct ModuleBlock
      {
        uint16_t Address;
        uint16_t Size;
        int16_t Offset;
      } PACK_POST;
    }
  #ifdef USE_PRAGMA_PACK
  #pragma pack(pop)
  #endif

    static_assert(sizeof(Header) == 0x14, "Invalid layout");
    
    const std::size_t MAX_SIZE = 131072;

    class Parser
    {
    public:
      explicit Parser(Binary::View data)
        : Data(data)
        , Ranges(RangeChecker::CreateSimple(Data.Size()))
        , Start(Data.As<uint8_t>())
        , Finish(Start + Data.Size())
      {
      }

      template<class T>
      const T& GetField(std::size_t offset) const
      {
        const auto* res = safe_ptr_cast<const T*>(Start);
        Require(res != nullptr);
        Require(Ranges->AddRange(offset, sizeof(T)));
        return *res;
      }

      template<class T>
      const T& PeekField(const int16_t* beField, std::size_t idx = 0) const
      {
        const uint8_t* const result = GetPointer(beField) + sizeof(T) * idx;
        Require(result + sizeof(T) <= Finish);
        return *safe_ptr_cast<const T*>(result);
      }

      template<class T>
      const T& GetField(const int16_t* beField, std::size_t idx = 0) const
      {
        const uint8_t* const result = GetPointer(beField) + sizeof(T) * idx;
        const std::size_t offset = result - Start;
        Require(Ranges->AddRange(offset, sizeof(T)));
        return *safe_ptr_cast<const T*>(result);
      }

      String GetString(const int16_t* beOffset) const
      {
        const uint8_t* const strStart = GetPointer(beOffset);
        Require(strStart < Finish);
        const uint8_t* const strEnd = std::find(strStart, Finish, 0);
        Require(Ranges->AddRange(strStart - Start, strEnd - strStart + 1));
        const StringView str(safe_ptr_cast<const char*>(strStart), strEnd - strStart);
        return Strings::OptimizeAscii(str);
      }

      Binary::View GetBlob(const int16_t* beOffset, std::size_t size) const
      {
        const uint8_t* const ptr = GetPointerNocheck(beOffset);
        if (ptr < Start || ptr >= Finish)
        {
          Dbg("Out of range %1%..%2% (%3%)", static_cast<const void*>(Start), static_cast<const void*>(Finish), static_cast<const void*>(ptr));
          return Binary::View(nullptr, 0);
        }
        const std::size_t offset = ptr - Start;
        const std::size_t maxSize = Finish - ptr;
        const std::size_t effectiveSize = std::min(size, maxSize);
        Require(Ranges->AddRange(offset, effectiveSize));
        return Data.SubView(offset, effectiveSize);
      }

      std::size_t GetSize() const
      {
        return Ranges->GetAffectedRange().second;
      }
    private:
      const uint8_t* GetPointerNocheck(const int16_t* beField) const
      {
        const int16_t relOffset = fromBE(*beField);
        return safe_ptr_cast<const uint8_t*>(beField) + relOffset;
      }

      const uint8_t* GetPointer(const int16_t* beField) const
      {
        const uint8_t* const result = GetPointerNocheck(beField);
        Require(result >= Start);
        return result;
      }
    private:
      const Binary::View Data;
      const RangeChecker::Ptr Ranges;
      const uint8_t* const Start;
      const uint8_t* const Finish;
    };

    class StubBuilder : public Builder
    {
    public:
      void SetTitle(String /*title*/) override {}
      void SetAuthor(String /*author*/) override {}
      void SetComment(String /*comment*/) override {}
      void SetDuration(uint_t /*total*/, uint_t /*fadeout*/) override {}
      void SetRegisters(uint16_t /*reg*/, uint16_t /*sp*/) override {}
      void SetRoutines(uint16_t /*init*/, uint16_t /*play*/) override {}
      void AddBlock(uint16_t /*addr*/, Binary::View /*data*/) override {}
    };

    const StringView HEADER_FORMAT(
      "'Z'X'A'Y" // uint8_t Signature[4];
      "'E'M'U'L" // only one type is supported now
      "??"       // versions
      "??"       // player offset
      "??"       // author offset
      "??"       // misc offset
      "00"       // first module
      "00"       // last module
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(HEADER_FORMAT))
      {
      }

      String GetDescription() const override
      {
        return Text::AY_EMUL_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return GetModulesCount(rawData) == 1;
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, 0, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    class MemoryDumpBuilder : public BlobBuilder
    {
      class Im1Player
      {
      public:
        Im1Player(uint16_t init, uint16_t introutine)
        {
          static const uint8_t PLAYER_TEMPLATE[] =
          {
            0xf3, //di
            0xcd, 0, 0, //call init (+2)
            0xed, 0x56, //loop: im 1
            0xfb, //ei
            0x76, //halt
            0xcd, 0, 0, //call routine (+9)
            0x18, 0xf7 //jr loop
          };
          static_assert(sizeof(Im1Player) == sizeof(PLAYER_TEMPLATE), "Invalid layout");
          std::copy(PLAYER_TEMPLATE, std::end(PLAYER_TEMPLATE), Data.begin());
          Data[0x2] = init & 0xff;
          Data[0x3] = init >> 8;
          Data[0x9] = introutine & 0xff;
          Data[0xa] = introutine >> 8; //call routine
        }

        std::array<uint8_t, 13> Data;
      };

      class Im2Player
      {
      public:
        explicit Im2Player(uint16_t init)
        {
          static const uint8_t PLAYER_TEMPLATE[] =
          {
            0xf3, //di
            0xcd, 0, 0, //call init (+2)
            0xed, 0x5e, //loop: im 2
            0xfb, //ei
            0x76, //halt
            0x18, 0xfa //jr loop
          };
          static_assert(sizeof(Im2Player) == sizeof(PLAYER_TEMPLATE), "Invalid layout");
          std::copy(PLAYER_TEMPLATE, std::end(PLAYER_TEMPLATE), Data.begin());
          Data[0x2] = init & 0xff;
          Data[0x3] = init >> 8;
        }

        std::array<uint8_t, 10> Data;
      };
    public:
      void SetTitle(String /*title*/) override
      {
      }

      void SetAuthor(String /*author*/) override
      {
      }

      void SetComment(String /*comment*/) override
      {
      }

      void SetDuration(uint_t /*total*/, uint_t /*fadeout*/) override
      {
      }

      void SetRegisters(uint16_t /*reg*/, uint16_t /*sp*/) override
      {
      }

      void SetRoutines(uint16_t init, uint16_t play) override
      {
        assert(init);
        if (play)
        {
          Im1Player player(init, play);
          AddBlock(0, player.Data);
        }
        else
        {
          Im2Player player(init);
          AddBlock(0, player.Data);
        }
      }

      void AddBlock(uint16_t addr, Binary::View block) override
      {
        Dump& data = AllocateData();
        const std::size_t toCopy = std::min(block.Size(), data.size() - addr);
        std::memcpy(&data[addr], block.Start(), toCopy);
      }

      Binary::Container::Ptr Result() const override
      {
        return Data
          ? Binary::CreateContainer(Data, 0, Data->size())
          : Binary::Container::Ptr();
      }
    private:
      Dump& AllocateData()
      {
        if (!Data)
        {
          Data.reset(new Dump(65536));
          InitializeBlock(0xc9, 0, 0x100);
          InitializeBlock(0xff, 0x100, 0x3f00);
          InitializeBlock(uint8_t(0x00), 0x4000, 0xc000);
          Data->at(0x38) = 0xfb;
        }
        return *Data;
      }

      void InitializeBlock(uint8_t src, std::size_t offset, std::size_t size)
      {
        Dump& data = *Data;
        const std::size_t toFill = std::min(size, data.size() - offset);
        std::memset(&data[offset], src, toFill);
      }
    private:
      std::shared_ptr<Dump> Data;
    };

    class FileBuilder : public BlobBuilder
    {
      //as a container
      class VariableDump : public Dump
      {
      public:
        VariableDump()
        {
          reserve(MAX_SIZE);
        }

        template<class T>
        T* Add(const T& obj)
        {
          return static_cast<T*>(Add(&obj, sizeof(obj)));
        }

        char* Add(const String& str)
        {
          return static_cast<char*>(Add(str.c_str(), str.size() + 1));
        }

        void* Add(const void* src, std::size_t srcSize)
        {
          const std::size_t prevSize = size();
          Require(prevSize + srcSize <= capacity());
          resize(prevSize + srcSize);
          void* const dst = data() + prevSize;
          std::memcpy(dst, src, srcSize);
          return dst;
        }
      };

      template<class T>
      static void SetPointer(int16_t* ptr, const T obj)
      {
        static_assert(std::is_pointer<T>::value, "Should be pointer");
        const std::ptrdiff_t offset = safe_ptr_cast<const uint8_t*>(obj) - safe_ptr_cast<const uint8_t*>(ptr);
        assert(offset > 0);//layout data sequentally
        *ptr = fromBE<int16_t>(static_cast<uint16_t>(offset));
      }
    public:
      FileBuilder()
        : Duration(), Fadeout()
        , Register(), StackPointer()
        , InitRoutine(), PlayRoutine()
      {
      }

      void SetTitle(String title) override
      {
        Title = std::move(title);
      }

      void SetAuthor(String author) override
      {
        Author = std::move(author);
      }

      void SetComment(String comment) override
      {
        Comment = std::move(comment);
      }

      void SetDuration(uint_t total, uint_t fadeout) override
      {
        Duration = static_cast<uint16_t>(total);
        Fadeout = static_cast<uint16_t>(fadeout);
      }

      void SetRegisters(uint16_t reg, uint16_t sp) override
      {
        Register = reg;
        StackPointer = sp;
      }

      void SetRoutines(uint16_t init, uint16_t play) override
      {
        InitRoutine = init;
        PlayRoutine = play;
      }

      void AddBlock(uint16_t addr, Binary::View block) override
      {
        const auto* fromCopy = block.As<uint8_t>();
        const std::size_t toCopy = std::min(block.Size(), std::size_t(0x10000 - addr));
        Blocks.push_back(BlocksList::value_type(addr, Dump(fromCopy, fromCopy + toCopy)));
      }

      Binary::Container::Ptr Result() const override
      {
        std::unique_ptr<VariableDump> result(new VariableDump());
        //init header
        Header* const header = result->Add(Header());
        std::memset(header, 0, sizeof(*header));
        std::copy(SIGNATURE, std::end(SIGNATURE), header->Signature);
        std::copy(EMUL::SIGNATURE, std::end(EMUL::SIGNATURE), header->Type);
        SetPointer(&header->AuthorOffset, result->Add(Author));
        SetPointer(&header->MiscOffset, result->Add(Comment));
        //init descr
        ModuleDescription* const descr = result->Add(ModuleDescription());
        SetPointer(&header->DescriptionsOffset, descr);
        SetPointer(&descr->TitleOffset, result->Add(Title));
        //init data
        EMUL::ModuleData* const data = result->Add(EMUL::ModuleData());
        SetPointer(&descr->DataOffset, data);
        data->TotalLength = fromBE(Duration);
        data->FadeLength = fromBE(Fadeout);
        data->RegValue = fromBE(Register);
        //init pointers
        EMUL::ModulePointers* const pointers = result->Add(EMUL::ModulePointers());
        SetPointer(&data->PointersOffset, pointers);
        pointers->SP = fromBE(StackPointer);
        pointers->InitAddr = fromBE(InitRoutine);
        pointers->PlayAddr = fromBE(PlayRoutine);
        //init blocks
        std::list<EMUL::ModuleBlock*> blockPtrs;
        //all blocks + limiter
        for (uint_t block = 0; block != Blocks.size() + 1; ++block)
        {
          blockPtrs.push_back(result->Add(EMUL::ModuleBlock()));
        }
        SetPointer(&data->BlocksOffset, blockPtrs.front());
        //fill blocks
        for (auto it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it, blockPtrs.pop_front())
        {
          EMUL::ModuleBlock* const dst = blockPtrs.front();
          dst->Address = fromBE<uint16_t>(it->first);
          dst->Size = fromBE<uint16_t>(static_cast<uint16_t>(it->second.size()));
          SetPointer(&dst->Offset, result->Add(it->second.data(), it->second.size()));
          Dbg("Stored block %1% bytes at %2% stored at %3%", fromBE(dst->Size), fromBE(dst->Address), fromBE(dst->Offset));
        }
        return Binary::CreateContainer(std::unique_ptr<Dump>(std::move(result)));
      }
    private:
      String Title;
      String Author;
      String Comment;
      uint16_t Duration;
      uint16_t Fadeout;
      uint16_t Register;
      uint16_t StackPointer;
      uint16_t InitRoutine;
      uint16_t PlayRoutine;
      typedef std::list<std::pair<uint16_t, Dump> > BlocksList;
      BlocksList Blocks;
    };

    uint_t GetModulesCount(Binary::View data)
    {
      if (const auto* header = data.As<Header>())
      {
        if (header->FirstModuleIndex > header->LastModuleIndex)
        {
          return 0;
        }
        if (0 != std::memcmp(header->Signature, SIGNATURE, sizeof(SIGNATURE)))
        {
          return 0;
        }
        if (0 != std::memcmp(header->Type, EMUL::SIGNATURE, sizeof(EMUL::SIGNATURE)))
        {
          return 0;
        }
        const int_t minOffset = sizeof(*header);
        const int_t maxOffset = data.Size();
        const int_t authorOffset = int_t(offsetof(Header, AuthorOffset)) + fromBE(header->AuthorOffset);
        if (!Math::InRange(authorOffset, minOffset, maxOffset))
        {
          return 0;
        }
        const int_t miscOffset = int_t(offsetof(Header, MiscOffset)) + fromBE(header->MiscOffset);
        //some of the tunes has improper offset
        if (miscOffset >= maxOffset)
        {
          return 0;
        }
        const int_t descrOffset = int_t(offsetof(Header, DescriptionsOffset)) + fromBE(header->DescriptionsOffset);
        if (descrOffset < minOffset)
        {
          return 0;
        }
        const std::size_t count = header->LastModuleIndex + 1;
        if (descrOffset + int_t(count * sizeof(ModuleDescription)) > maxOffset)
        {
          return 0;
        }
        return count;
      }
      return 0;
    }

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, std::size_t idx, Builder& target)
    {
      const Binary::View container(rawData);
      if (idx >= GetModulesCount(container))
      {
        return {};
      }
      try
      {
        Dbg("Parse idx=%1%, totalSize=%2%", idx, container.Size());
        const Parser data(container);
        const auto& header = data.GetField<Header>(std::size_t(0));
        target.SetAuthor(data.GetString(&header.AuthorOffset));
        target.SetComment(data.GetString(&header.MiscOffset));
        const ModuleDescription& description = data.GetField<ModuleDescription>(&header.DescriptionsOffset, idx);
        target.SetTitle(data.GetString(&description.TitleOffset));

        const auto& moddata = data.GetField<EMUL::ModuleData>(&description.DataOffset);
        if (const uint_t duration = fromBE(moddata.TotalLength))
        {
          target.SetDuration(duration, fromBE(moddata.FadeLength));
        }
        const auto& modpointers = data.GetField<EMUL::ModulePointers>(&moddata.PointersOffset);
        target.SetRegisters(fromBE(moddata.RegValue), fromBE(modpointers.SP));
        const auto& firstBlock = data.GetField<EMUL::ModuleBlock>(&moddata.BlocksOffset);
        target.SetRoutines(fromBE(modpointers.InitAddr ? modpointers.InitAddr : firstBlock.Address), fromBE(modpointers.PlayAddr));
        uint32_t crc = 0;
        std::size_t blocksSize = 0;
        for (std::size_t blockIdx = 0; ; ++blockIdx)
        {
          if (!data.PeekField<uint16_t>(&moddata.BlocksOffset, 3 * blockIdx))
          {
            break;
          }
          const auto& block = data.GetField<EMUL::ModuleBlock>(&moddata.BlocksOffset, blockIdx);
          const uint16_t blockAddr = fromBE(block.Address);
          const std::size_t blockSize = fromBE(block.Size);
          Dbg("Block %1% bytes at %2% located at %3%", blockSize, blockAddr, fromBE(block.Offset));
          if (const auto blockData = data.GetBlob(&block.Offset, blockSize))
          {
            target.AddBlock(blockAddr, blockData);
            crc = Binary::Crc32(blockData, crc);
            blocksSize += blockData.Size();
          }
          Require(blocksSize < MAX_SIZE);
        }
        auto containerData = rawData.GetSubcontainer(0, data.GetSize());
        return CreateKnownCrcContainer(std::move(containerData), crc);
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    BlobBuilder::Ptr CreateMemoryDumpBuilder()
    {
      return MakePtr<MemoryDumpBuilder>();
    }

    BlobBuilder::Ptr CreateFileBuilder()
    {
      return MakePtr<FileBuilder>();
    }
  } //namespace AY

  Decoder::Ptr CreateAYEMULDecoder()
  {
    return MakePtr<AY::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
