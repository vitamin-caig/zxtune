/**
 *
 * @file
 *
 * @brief  AYC support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ayc.h"

#include "formats/chiptune/container.h"

#include <binary/dump.h>
#include <binary/format_factories.h>

#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>

#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace AYC
  {
    const std::size_t MAX_REGISTERS = 14;

    struct BufferDescription
    {
      uint8_t SizeHi;
      le_uint16_t Offset;

      std::size_t GetAbsoluteOffset(uint_t idx) const
      {
        return Offset + idx * sizeof(*this) + 4;
      }
    };

    struct Header
    {
      le_uint16_t Duration;
      std::array<BufferDescription, MAX_REGISTERS> Buffers;
      uint8_t Reserved[6];
    };

    static_assert(sizeof(BufferDescription) * alignof(BufferDescription) == 3, "Invalid layout");
    static_assert(sizeof(Header) * alignof(Header) == 50, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header) + 14;

    class StubBuilder : public Builder
    {
    public:
      void SetFrames(std::size_t /*count*/) override {}
      void StartChannel(uint_t /*idx*/) override {}
      void AddValues(Binary::View /*values*/) override {}
    };

    bool FastCheck(Binary::View rawData)
    {
      if (rawData.Size() <= sizeof(Header))
      {
        return false;
      }
      const auto& header = *rawData.As<Header>();
      std::size_t minDataStart = sizeof(header);
      for (uint_t reg = 0; reg != header.Buffers.size(); ++reg)
      {
        const BufferDescription& buf = header.Buffers[reg];
        if (buf.SizeHi != 1 && buf.SizeHi != 4)
        {
          return false;
        }
        const std::size_t dataOffset = buf.GetAbsoluteOffset(reg);
        if (dataOffset < minDataStart)
        {
          return false;
        }
        minDataStart = dataOffset;
      }
      return true;
    }

    const auto DESCRIPTION = "CPC AYC"sv;
    const auto FORMAT =
        "?00-75"              // 10 min approx
        "01|04 2e00"          // assume first chunk is right after header
        "(01|04 ?00-80){13}"  // no more than 32k
        "ff{6}"
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

    class Stream
    {
    public:
      Stream(uint_t sizeHi, const uint8_t* begin, const uint8_t* end)
        : SizeHi(sizeHi)
        , Cursor(begin)
        , Limit(end)
      {}

      uint_t GetBufferSize() const
      {
        return SizeHi << 8;
      }

      uint8_t ReadByte()
      {
        Require(Cursor < Limit);
        return *Cursor++;
      }

      uint_t ReadCounter()
      {
        const auto val = static_cast<uint8_t>(-ReadByte());
        return val ? val : 256;
      }

      std::size_t ReadBackRef()
      {
        const std::size_t lo = ReadByte();
        return SizeHi == 1 ? lo : lo | 256 * ReadByte();
      }

      const uint8_t* GetCursor() const
      {
        return Cursor;
      }

    private:
      const uint_t SizeHi;
      const uint8_t* Cursor;
      const uint8_t* const Limit;
    };

    void ParseBuffer(uint_t count, Stream& source, Builder& target)
    {
      const std::size_t bufSize = source.GetBufferSize();
      Binary::Dump buf(bufSize);
      std::size_t cursor = 0;
      uint_t flag = 0x40;
      // dX_flag
      while (count)
      {
        // dX_next
        flag <<= 1;
        if ((flag & 0xff) == 0)
        {
          flag = source.ReadByte();
          flag = (flag << 1) | 1;
        }
        if ((flag & 0x100) != 0)
        {
          flag &= 0xff;
          uint_t counter = source.ReadCounter();
          std::size_t srcPtr = source.ReadBackRef();
          Require(count >= counter);
          Require(srcPtr < bufSize);
          for (count -= counter; counter; --counter)
          {
            buf[cursor++] = buf[srcPtr++];
            if (cursor >= bufSize)
            {
              target.AddValues(buf);
              cursor -= bufSize;
            }
            if (srcPtr >= bufSize)
            {
              srcPtr -= bufSize;
            }
          }
        }
        else
        {
          // dX_chr
          --count;
          buf[cursor++] = source.ReadByte();
          if (cursor >= bufSize)
          {
            target.AddValues(buf);
            cursor -= bufSize;
          }
        }
      }
      if (cursor)
      {
        buf.resize(cursor);
        target.AddValues(buf);
      }
    }

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
      }

      try
      {
        const std::size_t size = data.Size();
        const auto* begin = data.As<uint8_t>();
        const uint8_t* const end = begin + size;
        const auto& header = *data.As<Header>();
        const uint_t frames = header.Duration;
        target.SetFrames(frames);
        std::size_t usedBegin = size;
        std::size_t usedEnd = 0;
        for (uint_t reg = 0; reg != header.Buffers.size(); ++reg)
        {
          target.StartChannel(reg);
          const BufferDescription& buf = header.Buffers[reg];
          const std::size_t offset = buf.GetAbsoluteOffset(reg);
          Require(offset < size);
          Stream stream(buf.SizeHi, begin + offset, end);
          ParseBuffer(frames, stream, target);
          usedBegin = std::min(usedBegin, offset);
          usedEnd = std::max<std::size_t>(usedEnd, stream.GetCursor() - begin);
        }
        auto subData = rawData.GetSubcontainer(0, usedEnd);
        return CreateCalculatingCrcContainer(std::move(subData), usedBegin, usedEnd - usedBegin);
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
  }  // namespace AYC

  Decoder::Ptr CreateAYCDecoder()
  {
    return MakePtr<AYC::Decoder>();
  }
}  // namespace Formats::Chiptune
