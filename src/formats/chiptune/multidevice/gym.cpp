/**
 *
 * @file
 *
 * @brief  GYM support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <formats/chiptune/container.h>

#include <binary/format_factories.h>
#include <math/numeric.h>

#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>

#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace GYM
  {
    const auto DESCRIPTION = "Genesis YM2612 (unpacked)"sv;

    using SignatureType = std::array<uint8_t, 4>;
    using StringType = std::array<uint8_t, 32>;

    struct RawHeader
    {
      SignatureType Signature;
      StringType Song;
      StringType Game;
      StringType Copyright;
      StringType Emulator;
      StringType Dumper;
      std::array<uint8_t, 256> Comment;
      le_uint32_t LoopStart;
      le_uint32_t PackedSize;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 428, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader) + 256;
    const std::size_t MAX_SIZE = 16 * 1024 * 1024;

    const auto FORMAT =
        "'G'Y'M'X"  // signature
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
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
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        const std::size_t realSize = std::min(rawData.Size(), MAX_SIZE);
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, realSize);
        return CreateCalculatingCrcContainer(data, 0, realSize);
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace GYM

  Decoder::Ptr CreateGYMDecoder()
  {
    return MakePtr<GYM::Decoder>();
  }
}  // namespace Formats::Chiptune
