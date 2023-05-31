/**
 *
 * @file
 *
 * @brief  AY/EMUL support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/emulation/ay.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
#include <range_checker.h>
// library includes
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <debug/log.h>
#include <formats/chiptune.h>
#include <math/numeric.h>
#include <strings/optimize.h>
// std includes
#include <array>
#include <cstring>
#include <list>
#include <type_traits>

namespace Formats::Chiptune
{
  namespace AY
  {
    const Debug::Stream Dbg("Formats::Chiptune::AY");

    const uint8_t SIGNATURE[] = {'Z', 'X', 'A', 'Y'};

    struct Header
    {
      uint8_t Signature[4];  // ZXAY
      uint8_t Type[4];
      uint8_t FileVersion;
      uint8_t PlayerVersion;
      be_int16_t SpecialPlayerOffset;
      be_int16_t AuthorOffset;
      be_int16_t MiscOffset;
      uint8_t LastModuleIndex;
      uint8_t FirstModuleIndex;
      be_int16_t DescriptionsOffset;
    };

    struct ModuleDescription
    {
      be_int16_t TitleOffset;
      be_int16_t DataOffset;
    };

    namespace EMUL
    {
      const uint8_t SIGNATURE[] = {'E', 'M', 'U', 'L'};

      struct ModuleData
      {
        uint8_t AmigaChannelsMapping[4];
        be_uint16_t TotalLength;
        be_uint16_t FadeLength;
        be_uint16_t RegValue;
        be_int16_t PointersOffset;
        be_int16_t BlocksOffset;
      };

      struct ModulePointers
      {
        be_uint16_t SP;
        be_uint16_t InitAddr;
        be_uint16_t PlayAddr;
      };

      struct ModuleBlock
      {
        be_uint16_t Address;
        be_uint16_t Size;
        be_int16_t Offset;
      };
    }  // namespace EMUL

    static_assert(sizeof(Header) * alignof(Header) == 0x14, "Invalid layout");
    static_assert(sizeof(ModuleDescription) * alignof(ModuleDescription) == 4, "Invalid layout");
    static_assert(sizeof(EMUL::ModuleData) * alignof(EMUL::ModuleData) == 14, "Invalid layout");
    static_assert(sizeof(EMUL::ModulePointers) * alignof(EMUL::ModulePointers) == 6, "Invalid layout");
    static_assert(sizeof(EMUL::ModuleBlock) * alignof(EMUL::ModuleBlock) == 6, "Invalid layout");

    const std::size_t MIN_STRING_SIZE = 3;
    const std::size_t MIN_SIZE = sizeof(Header) + 3 * MIN_STRING_SIZE  // author, comment, title
                                 + sizeof(ModuleDescription) + sizeof(EMUL::ModuleData) + sizeof(EMUL::ModulePointers)
                                 + sizeof(EMUL::ModuleBlock);

    const std::size_t MAX_SIZE = 131072;

    class Parser
    {
    public:
      explicit Parser(Binary::View data)
        : Data(data)
        , Ranges(RangeChecker::CreateSimple(Data.Size()))
        , Start(Data.As<uint8_t>())
        , Finish(Start + Data.Size())
      {}

      template<class T>
      const T& GetField(std::size_t offset) const
      {
        const auto* res = safe_ptr_cast<const T*>(Start);
        Require(res != nullptr);
        Require(Ranges->AddRange(offset, sizeof(T)));
        return *res;
      }

      template<class T>
      const T& PeekField(const be_int16_t* beField, std::size_t idx = 0) const
      {
        const uint8_t* const result = GetPointer(beField) + sizeof(T) * idx;
        Require(result + sizeof(T) <= Finish);
        return *safe_ptr_cast<const T*>(result);
      }

      template<class T>
      const T& GetField(const be_int16_t* beField, std::size_t idx = 0) const
      {
        const uint8_t* const result = GetPointer(beField) + sizeof(T) * idx;
        const std::size_t offset = result - Start;
        Require(Ranges->AddRange(offset, sizeof(T)));
        return *safe_ptr_cast<const T*>(result);
      }

      String GetString(const be_int16_t* beOffset) const
      {
        const uint8_t* const strStart = GetPointer(beOffset);
        Require(strStart < Finish);
        const uint8_t* const strEnd = std::find(strStart, Finish, 0);
        Require(Ranges->AddRange(strStart - Start, strEnd - strStart + 1));
        const StringView str(safe_ptr_cast<const char*>(strStart), strEnd - strStart);
        return Strings::OptimizeAscii(str);
      }

      Binary::View GetBlob(const be_int16_t* beOffset, std::size_t size) const
      {
        const uint8_t* const ptr = GetPointerNocheck(beOffset);
        if (ptr < Start || ptr >= Finish)
        {
          Dbg("Out of range {}..{} ({})", static_cast<const void*>(Start), static_cast<const void*>(Finish),
              static_cast<const void*>(ptr));
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
      static const uint8_t* GetPointerNocheck(const be_int16_t* beField)
      {
        const int16_t relOffset = *beField;
        return safe_ptr_cast<const uint8_t*>(beField) + relOffset;
      }

      const uint8_t* GetPointer(const be_int16_t* beField) const
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
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetDuration(uint_t /*total*/, uint_t /*fadeout*/) override {}
      void SetRegisters(uint16_t /*reg*/, uint16_t /*sp*/) override {}
      void SetRoutines(uint16_t /*init*/, uint16_t /*play*/) override {}
      void AddBlock(uint16_t /*addr*/, Binary::View /*data*/) override {}
    };

    const auto HEADER_FORMAT =
        "'Z'X'A'Y"  // uint8_t Signature[4];
        "'E'M'U'L"  // only one type is supported now
        "??"        // versions
        "??"        // player offset
        "??"        // author offset
        "??"        // misc offset
        "00"        // first module
        "00"        // last module
        ""_sv;

    const Char DESCRIPTION[] = "AY/EMUL";

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(HEADER_FORMAT, MIN_SIZE))
      {}

      String GetDescription() const override
      {
        return DESCRIPTION;
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
          static const uint8_t PLAYER_TEMPLATE[] = {
              0xf3,           // di
              0xcd, 0,    0,  // call init (+2)
              0xed, 0x56,     // loop: im 1
              0xfb,           // ei
              0x76,           // halt
              0xcd, 0,    0,  // call routine (+9)
              0x18, 0xf7      // jr loop
          };
          static_assert(sizeof(Im1Player) * alignof(Im1Player) * alignof(Im1Player) == sizeof(PLAYER_TEMPLATE),
                        "Invalid layout");
          std::copy(PLAYER_TEMPLATE, std::end(PLAYER_TEMPLATE), Data.begin());
          Data[0x2] = init & 0xff;
          Data[0x3] = init >> 8;
          Data[0x9] = introutine & 0xff;
          Data[0xa] = introutine >> 8;  // call routine
        }

        std::array<uint8_t, 13> Data;
      };

      class Im2Player
      {
      public:
        explicit Im2Player(uint16_t init)
        {
          static const uint8_t PLAYER_TEMPLATE[] = {
              0xf3,           // di
              0xcd, 0,    0,  // call init (+2)
              0xed, 0x5e,     // loop: im 2
              0xfb,           // ei
              0x76,           // halt
              0x18, 0xfa      // jr loop
          };
          static_assert(sizeof(Im2Player) * alignof(Im2Player) * alignof(Im2Player) == sizeof(PLAYER_TEMPLATE),
                        "Invalid layout");
          std::copy(PLAYER_TEMPLATE, std::end(PLAYER_TEMPLATE), Data.begin());
          Data[0x2] = init & 0xff;
          Data[0x3] = init >> 8;
        }

        std::array<uint8_t, 10> Data;
      };

    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetDuration(uint_t /*total*/, uint_t /*fadeout*/) override {}

      void SetRegisters(uint16_t /*reg*/, uint16_t /*sp*/) override {}

      void SetRoutines(uint16_t init, uint16_t play) override
      {
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
        auto& data = AllocateData();
        const auto toCopy = block.SubView(0, data.Size() - addr);
        std::memcpy(data.Get(addr), toCopy.Start(), toCopy.Size());
      }

      Binary::Container::Ptr Result() const override
      {
        return Data.CaptureResult();
      }

    private:
      Binary::DataBuilder& AllocateData()
      {
        if (!Data.Size())
        {
          Data.Allocate(65536);
          InitializeBlock(0xc9, 0, 0x100);
          InitializeBlock(0xff, 0x100, 0x3f00);
          InitializeBlock(uint8_t(0x00), 0x4000, 0xc000);
          Data.Get<uint8_t>(0x38) = 0xfb;
        }
        return Data;
      }

      void InitializeBlock(uint8_t src, std::size_t offset, std::size_t size)
      {
        auto& data = AllocateData();
        const std::size_t toFill = std::min(size, data.Size() - offset);
        std::memset(data.Get(offset), src, toFill);
      }

    private:
      mutable Binary::DataBuilder Data;
    };

    class FileBuilder
      : public BlobBuilder
      , private MetaBuilder
    {
      template<class T>
      static void SetPointer(be_int16_t* ptr, const T obj)
      {
        static_assert(std::is_pointer<T>::value, "Should be pointer");
        const std::ptrdiff_t offset = safe_ptr_cast<const uint8_t*>(obj) - safe_ptr_cast<const uint8_t*>(ptr);
        assert(offset > 0);  // layout data sequentally
        *ptr = static_cast<int16_t>(offset);
      }

    public:
      FileBuilder() = default;

      void SetTitle(StringView title) override
      {
        Title = title.to_string();
      }

      void SetAuthor(StringView author) override
      {
        Author = author.to_string();
      }

      void SetComment(StringView comment) override
      {
        Comment = comment.to_string();
      }

      void SetProgram(StringView /*program*/) override {}
      void SetStrings(const Strings::Array& /*strings*/) override {}

      MetaBuilder& GetMetaBuilder() override
      {
        return *this;
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
        Blocks.emplace_back(addr, Binary::CreateContainer(block.SubView(0, std::size_t(0x10000 - addr))));
      }

      Binary::Container::Ptr Result() const override
      {
        Binary::DataBuilder result(MAX_SIZE);
        // init header
        auto& header = result.Add<Header>();
        std::copy(SIGNATURE, std::end(SIGNATURE), header.Signature);
        std::copy(EMUL::SIGNATURE, std::end(EMUL::SIGNATURE), header.Type);
        SetPointer(&header.AuthorOffset, result.AddCString(Author));
        SetPointer(&header.MiscOffset, result.AddCString(Comment));
        // init descr
        auto& descr = result.Add<ModuleDescription>();
        SetPointer(&header.DescriptionsOffset, &descr);
        SetPointer(&descr.TitleOffset, result.AddCString(Title));
        // init data
        auto& data = result.Add<EMUL::ModuleData>();
        SetPointer(&descr.DataOffset, &data);
        data.TotalLength = Duration;
        data.FadeLength = Fadeout;
        data.RegValue = Register;
        // init pointers
        auto& pointers = result.Add<EMUL::ModulePointers>();
        SetPointer(&data.PointersOffset, &pointers);
        pointers.SP = StackPointer;
        pointers.InitAddr = InitRoutine;
        pointers.PlayAddr = PlayRoutine;
        // init blocks
        std::list<EMUL::ModuleBlock*> blockPtrs;
        // all blocks + limiter
        for (uint_t block = 0; block != Blocks.size() + 1; ++block)
        {
          blockPtrs.push_back(&result.Add<EMUL::ModuleBlock>());
        }
        SetPointer(&data.BlocksOffset, blockPtrs.front());
        // fill blocks
        for (auto it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it, blockPtrs.pop_front())
        {
          auto* const dst = blockPtrs.front();
          dst->Address = it->first;
          dst->Size = static_cast<uint16_t>(it->second->Size());
          SetPointer(&dst->Offset, result.Add(*it->second));
          Dbg("Stored block {} bytes at {} stored at {}", dst->Size, dst->Address, dst->Offset);
        }
        return result.CaptureResult();
      }

    private:
      String Title;
      String Author;
      String Comment;
      uint16_t Duration = 0;
      uint16_t Fadeout = 0;
      uint16_t Register = 0;
      uint16_t StackPointer = 0;
      uint16_t InitRoutine = 0;
      uint16_t PlayRoutine = 0;
      using BlocksList = std::list<std::pair<uint16_t, Binary::Data::Ptr>>;
      BlocksList Blocks;
    };

    uint_t GetModulesCount(Binary::View data)
    {
      if (data.Size() < MIN_SIZE)
      {
        return 0;
      }
      const auto* header = data.As<Header>();
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
      const int_t authorOffset = int_t(offsetof(Header, AuthorOffset)) + header->AuthorOffset;
      if (!Math::InRange(authorOffset, minOffset, maxOffset))
      {
        return 0;
      }
      const int_t miscOffset = int_t(offsetof(Header, MiscOffset)) + header->MiscOffset;
      // some of the tunes has improper offset
      if (miscOffset >= maxOffset)
      {
        return 0;
      }
      const int_t descrOffset = int_t(offsetof(Header, DescriptionsOffset)) + header->DescriptionsOffset;
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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, std::size_t idx, Builder& target)
    {
      const Binary::View container(rawData);
      if (idx >= GetModulesCount(container))
      {
        return {};
      }
      try
      {
        Dbg("Parse idx={}, totalSize={}", idx, container.Size());
        const Parser data(container);
        const auto& header = data.GetField<Header>(std::size_t(0));
        auto& meta = target.GetMetaBuilder();
        meta.SetAuthor(data.GetString(&header.AuthorOffset));
        meta.SetComment(data.GetString(&header.MiscOffset));
        const auto& description = data.GetField<ModuleDescription>(&header.DescriptionsOffset, idx);
        meta.SetTitle(data.GetString(&description.TitleOffset));

        const auto& moddata = data.GetField<EMUL::ModuleData>(&description.DataOffset);
        if (const uint_t duration = moddata.TotalLength)
        {
          target.SetDuration(duration, moddata.FadeLength);
        }
        const auto& modpointers = data.GetField<EMUL::ModulePointers>(&moddata.PointersOffset);
        target.SetRegisters(moddata.RegValue, modpointers.SP);
        const auto& firstBlock = data.GetField<EMUL::ModuleBlock>(&moddata.BlocksOffset);
        target.SetRoutines(modpointers.InitAddr ? modpointers.InitAddr : firstBlock.Address, modpointers.PlayAddr);
        uint32_t crc = 0;
        std::size_t blocksSize = 0;
        for (std::size_t blockIdx = 0;; ++blockIdx)
        {
          if (!data.PeekField<be_uint16_t>(&moddata.BlocksOffset, 3 * blockIdx))
          {
            break;
          }
          const auto& block = data.GetField<EMUL::ModuleBlock>(&moddata.BlocksOffset, blockIdx);
          const uint16_t blockAddr = block.Address;
          const std::size_t blockSize = block.Size;
          Dbg("Block {} bytes at {} located at {}", blockSize, blockAddr, block.Offset);
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
  }  // namespace AY

  Decoder::Ptr CreateAYEMULDecoder()
  {
    return MakePtr<AY::Decoder>();
  }
}  // namespace Formats::Chiptune
