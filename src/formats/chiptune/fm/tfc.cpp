/**
 *
 * @file
 *
 * @brief  TurboFM Compiled support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/fm/tfc.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <strings/sanitize.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace TFC
  {
    const auto DESCRIPTION = "TurboFM Compiled Dump"sv;

    using SignatureType = std::array<uint8_t, 6>;

    const SignatureType SIGNATURE = {{'T', 'F', 'M', 'c', 'o', 'm'}};

    struct RawHeader
    {
      SignatureType Sign;
      std::array<char, 3> Version;
      uint8_t IntFreq;
      std::array<le_uint16_t, 6> Offsets;
      uint8_t Reserved[12];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 34, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader) + 3 + 6;  // header + 3 empty strings + 6 finish markers
    const std::size_t MAX_STRING_SIZE = 64;
    const std::size_t MAX_COMMENT_SIZE = 384;

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetVersion(StringView /*version*/) override {}
      void SetIntFreq(uint_t /*freq*/) override {}

      void StartChannel(uint_t /*idx*/) override {}
      void StartFrame() override {}
      void SetSkip(uint_t /*count*/) override {}
      void SetLoop() override {}
      void SetSlide(uint_t /*slide*/) override {}
      void SetKeyOff() override {}
      void SetFreq(uint_t /*freq*/) override {}
      void SetRegister(uint_t /*reg*/, uint_t /*val*/) override {}
      void SetKeyOn() override {}
    };

    bool FastCheck(Binary::View rawData)
    {
      const std::size_t size = rawData.Size();
      if (size < MIN_SIZE)
      {
        return false;
      }
      const auto& hdr = *rawData.As<RawHeader>();
      return hdr.Sign == SIGNATURE
             && hdr.Offsets.end()
                    == std::find_if(hdr.Offsets.begin(), hdr.Offsets.end(), [size](auto o) { return o >= size; });
    }

    const auto FORMAT =
        "'T'F'M'c'o'm"
        "???"
        "32|3c"
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

    struct Context
    {
      std::size_t RetAddr = 0;
      std::size_t RepeatFrames = 0;

      Context() = default;
    };

    class Container
    {
    public:
      explicit Container(Binary::View data)
        : Data(data)
        , Min(Data.Size())
      {}

      std::size_t ParseFrameControl(std::size_t cursor, Builder& target, Context& context) const
      {
        if (context.RepeatFrames && !--context.RepeatFrames)
        {
          cursor = context.RetAddr;
          context.RetAddr = 0;
        }
        for (;;)
        {
          const uint_t cmd = Get<uint8_t>(cursor++);
          if (cmd == 0x7f)  //%01111111
          {
            return 0;
          }
          else if (0x7e == cmd)  //%01111110
          {
            target.SetLoop();
            continue;
          }
          else if (0xd0 == cmd)  //%11010000
          {
            Require(context.RepeatFrames == 0);
            Require(context.RetAddr == 0);
            context.RepeatFrames = Get<uint8_t>(cursor++);
            const int_t offset = Get<be_int16_t>(cursor);
            context.RetAddr = cursor += 2;
            return AdvanceCursor(cursor, offset);
          }
          else
          {
            return cursor - 1;
          }
        }
      }

      std::size_t ParseFrameCommands(std::size_t cursor, Builder& target) const
      {
        const uint_t cmd = Get<uint8_t>(cursor++);
        if (0xbf == cmd)  //%10111111
        {
          const int_t offset = Get<be_int16_t>(cursor);
          cursor += 2;
          ParseFrameData(AdvanceCursor(cursor, offset), target);
        }
        else if (0xff == cmd)  //%11111111
        {
          const int_t offset = -256 + Get<uint8_t>(cursor++);
          ParseFrameData(AdvanceCursor(cursor, offset), target);
        }
        else if (cmd >= 0xe0)  //%111ttttt
        {
          target.SetSkip(256 - cmd);
        }
        else if (cmd >= 0xc0)  //%110ddddd
        {
          target.SetSlide(cmd + 0x30);
        }
        else
        {
          cursor = ParseFrameData(cursor - 1, target);
        }
        return cursor;
      }

      std::size_t GetMin() const
      {
        return Min;
      }

      std::size_t GetMax() const
      {
        return Max;
      }

    private:
      static std::size_t AdvanceCursor(std::size_t cursor, std::ptrdiff_t offset)
      {
        // disable UB
        if (offset >= 0)
        {
          return cursor + offset;
        }
        else
        {
          const auto back = static_cast<std::size_t>(-offset);
          Require(cursor >= back);
          return cursor - back;
        }
      }

      std::size_t ParseFrameData(std::size_t cursor, Builder& target) const
      {
        const uint_t data = Get<uint8_t>(cursor++);
        if (0 != (data & 0xc0))
        {
          target.SetKeyOff();
        }
        if (0 != (data & 0x01))
        {
          const uint_t freq = Get<be_uint16_t>(cursor);
          cursor += 2;
          target.SetFreq(freq);
        }
        if (const uint_t regs = (data & 0x3e) >> 1)
        {
          for (uint_t i = 0; i != regs; ++i)
          {
            const uint_t reg = Get<uint8_t>(cursor++);
            const uint_t val = Get<uint8_t>(cursor++);
            target.SetRegister(reg, val);
          }
        }
        if (0 != (data & 0x80))
        {
          target.SetKeyOn();
        }
        return cursor;
      }

    private:
      template<class T>
      const T& Get(std::size_t offset) const
      {
        const T* const ptr = Data.SubView(offset).As<T>();
        Require(ptr != nullptr);
        Min = std::min(Min, offset);
        Max = std::max(Max, offset + sizeof(T));
        return *ptr;
      }

    private:
      const Binary::View Data;
      mutable std::size_t Min;
      mutable std::size_t Max = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
      }
      try
      {
        Binary::DataInputStream stream(data);
        const auto& header = stream.Read<RawHeader>();
        target.SetVersion({header.Version.data(), header.Version.size()});
        target.SetIntFreq(header.IntFreq);
        auto& meta = target.GetMetaBuilder();
        meta.SetTitle(Strings::Sanitize(stream.ReadCString(MAX_STRING_SIZE)));
        meta.SetAuthor(Strings::Sanitize(stream.ReadCString(MAX_STRING_SIZE)));
        meta.SetComment(Strings::SanitizeMultiline(stream.ReadCString(MAX_COMMENT_SIZE)));

        const Container container(data);
        for (uint_t chan = 0; chan != 6; ++chan)
        {
          target.StartChannel(chan);
          Context context;
          for (std::size_t cursor = header.Offsets[chan]; cursor != 0;)
          {
            if ((cursor = container.ParseFrameControl(cursor, target, context)))
            {
              target.StartFrame();
              cursor = container.ParseFrameCommands(cursor, target);
            }
          }
        }
        Require(container.GetMin() < container.GetMax());  // anything parsed

        const std::size_t usedSize = std::max(container.GetMax(), stream.GetPosition());
        const std::size_t fixedOffset = container.GetMin();
        auto subData = rawData.GetSubcontainer(0, usedSize);
        return CreateCalculatingCrcContainer(std::move(subData), fixedOffset, usedSize - fixedOffset);
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
  }  // namespace TFC

  Decoder::Ptr CreateTFCDecoder()
  {
    return MakePtr<TFC::Decoder>();
  }
}  // namespace Formats::Chiptune
