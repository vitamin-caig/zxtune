/**
 *
 * @file
 *
 * @brief  MUSE compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <error.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/compression/zlib_container.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/packed.h>
// std includes
#include <memory>

namespace Formats::Packed
{
  namespace Muse
  {
    const Debug::Stream Dbg("Formats::Packed::Muse");

    const auto DESCRIPTION = "MUSE Compressor"sv;
    const auto HEADER_PATTERN =
        "'M'U'S'E"
        "de ad be|ba af|be"
        "????"  // file size
        "????"  // crc
        "????"  // packed size
        "????"  // unpacked size
        ""sv;

    class Decoder : public Formats::Packed::Decoder
    {
    public:
      Decoder()
        : Depacker(Binary::CreateFormat(HEADER_PATTERN))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Depacker;
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Depacker->Match(rawData))
        {
          return {};
        }
        try
        {
          Binary::DataInputStream input(rawData);
          input.Skip(4);
          const uint_t sign = input.Read<be_uint32_t>();
          Require(sign == 0xdeadbabe || sign == 0xdeadbeaf);
          const std::size_t fileSize = input.Read<le_uint32_t>();
          /*const uint32_t crc = */ input.Read<le_uint32_t>();
          const std::size_t packedSize = input.Read<le_uint32_t>();
          const std::size_t unpackedSize = input.Read<le_uint32_t>();
          const auto packedData = input.ReadData(packedSize);
          auto unpackedData = Binary::Compression::Zlib::Decompress(packedData, unpackedSize);
          return CreateContainer(std::move(unpackedData), std::min<std::size_t>(fileSize, input.GetPosition()));
        }
        catch (const std::exception& e)
        {
          Dbg("Failed to uncompress: {}", e.what());
        }
        catch (const Error& e)
        {
          Dbg("Failed to uncompress: {}", e.ToString());
        }
        return {};
      }

    private:
      const Binary::Format::Ptr Depacker;
    };
  }  // namespace Muse

  Decoder::Ptr CreateMUSEDecoder()
  {
    return MakePtr<Muse::Decoder>();
  }
}  // namespace Formats::Packed
