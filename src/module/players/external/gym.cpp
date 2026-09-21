/**
 *
 * @file
 *
 * @brief  GYM format data decompression support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/gym.h"

#include "binary/compression/zlib.h"
#include "binary/container_factories.h"
#include "binary/data_builder.h"
#include "binary/input_stream.h"
#include "debug/log.h"

#include "byteorder.h"

namespace Module::GYM
{
  const Debug::Stream Dbg("Module::GYM");

  // TODO: rework, extract GYM parsing code to Formats library
  Binary::Data::Ptr CreateData(const Binary::Container& data)
  {
    Binary::DataInputStream input(data);
    const std::size_t unpackedSizeOffset = 424;
    const auto header = input.ReadData(unpackedSizeOffset);
    if (const auto unpackedSize = input.Read<le_uint32_t>())
    {
      Binary::DataBuilder output;
      output.Add(header);
      output.Add<le_uint32_t>(0);
      Binary::Compression::Zlib::Decompress(input.ReadRestData(), output);
      const auto realUnpackedSize = output.Size() - header.Size() - sizeof(unpackedSize);
      if (unpackedSize != realUnpackedSize)
      {
        Dbg("GYM unpacked size mismatch: {} -> {}", unpackedSize, realUnpackedSize);
      }
      return output.CaptureResult();
    }
    else
    {
      return data.GetSubcontainer(0, data.Size());
    }
  }
}  // namespace Module::GYM
