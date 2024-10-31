/**
 *
 * @file
 *
 * @brief  PSG support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/psg.h"

#include "formats/chiptune/container.h"

#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"

#include "make_ptr.h"
#include "string_view.h"

#include <cstring>
#include <utility>

namespace Formats::Chiptune
{
  namespace PSG
  {
    enum
    {
      MARKER = 0x1a,

      INT_BEGIN = 0xff,
      INT_SKIP = 0xfe,
      MUS_END = 0xfd
    };

    const uint8_t SIGNATURE[] = {'P', 'S', 'G'};

    struct Header
    {
      uint8_t Sign[3];
      uint8_t Marker;
      uint8_t Version;
      uint8_t Interrupt;
      uint8_t Padding[10];
    };

    static_assert(sizeof(Header) * alignof(Header) == 16, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header);

    class StubBuilder : public Builder
    {
    public:
      void AddChunks(std::size_t /*count*/) override {}
      void SetRegister(uint_t /*reg*/, uint_t /*val*/) override {}
    };

    bool FastCheck(Binary::View data)
    {
      if (data.Size() <= sizeof(Header))
      {
        return false;
      }
      const auto* header = data.As<Header>();
      return 0 == std::memcmp(header->Sign, SIGNATURE, sizeof(SIGNATURE)) && MARKER == header->Marker;
    }

    const auto DESCRIPTION = "Programmable Sound Generator"sv;
    const auto FORMAT =
        "'P'S'G"  // uint8_t Sign[3];
        "1a"      // uint8_t Marker;
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return FastCheck(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
      }

      const auto& header = *data.As<Header>();
      // workaround for some emulators
      const std::size_t offset = (header.Version == INT_BEGIN) ? offsetof(Header, Version) : sizeof(header);
      std::size_t restSize = data.Size() - offset;
      const uint8_t* bdata = data.As<uint8_t>() + offset;
      // detect as much chunks as possible, in despite of real format issues
      while (restSize)
      {
        const uint_t reg = *bdata;
        ++bdata;
        --restSize;
        if (INT_BEGIN == reg)
        {
          target.AddChunks(1);
        }
        else if (INT_SKIP == reg)
        {
          if (restSize < 1)
          {
            ++restSize;  // put byte back
            break;
          }
          target.AddChunks(4 * *bdata);
          ++bdata;
          --restSize;
        }
        else if (MUS_END == reg)
        {
          break;
        }
        else if (reg <= 15)  // register
        {
          if (restSize < 1)
          {
            ++restSize;  // put byte back
            break;
          }
          target.SetRegister(reg, *bdata);
          ++bdata;
          --restSize;
        }
        else
        {
          ++restSize;  // put byte back
          break;
        }
      }
      const std::size_t usedSize = data.Size() - restSize;
      auto subData = rawData.GetSubcontainer(0, usedSize);
      return CreateCalculatingCrcContainer(std::move(subData), offset, usedSize - offset);
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }  // namespace PSG

  Decoder::Ptr CreatePSGDecoder()
  {
    return MakePtr<PSG::Decoder>();
  }
}  // namespace Formats::Chiptune
