/**
 *
 * @file
 *
 * @brief  Zdata plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/src/location.h"

#include "binary/base64.h"
#include "binary/compression/zlib_container.h"
#include "binary/compression/zlib_stream.h"
#include "binary/crc.h"
#include "binary/data_builder.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "strings/prefixed_index.h"

#include "byteorder.h"
#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <array>

namespace ZXTune::Zdata
{
  const Debug::Stream Dbg("Core::ZData");

  const String PLUGIN_PREFIX("zdata:");

  using SignatureType = std::array<uint8_t, 2>;

  struct UInt24LE
  {
  public:
    UInt24LE()
      : Data()
    {}

    /*explicit*/ UInt24LE(uint_t val)
    {
      Require(0 == (val >> 24));
      Data[0] = val & 255;
      Data[1] = (val >> 8) & 255;
      Data[2] = val >> 16;
    }

    operator uint_t() const
    {
      return (Data[2] << 16) | (Data[1] << 8) | Data[0];
    }

  private:
    std::array<uint8_t, 3> Data;
  };

  // 4 LSBs of signature may be version
  const SignatureType SIGNATURE = {{0x64, 0x30}};

  struct RawMarker
  {
    SignatureType Signature;
    le_uint32_t Crc;
  };

  // size of data block, without header
  struct RawHeader : RawMarker
  {
    UInt24LE OriginalSize;
    UInt24LE PackedSize;
  };

  using TxtMarker = std::array<char, 8>;
  using TxtHeader = std::array<char, 16>;

  struct Marker
  {
    explicit Marker(uint32_t crc)
      : Value(crc)
    {}

    TxtMarker Encode() const
    {
      const RawMarker in = {SIGNATURE, Value};
      const auto* const inData = in.Signature.data();
      TxtMarker out;
      auto* const outData = out.data();
      Binary::Base64::Encode(inData, inData + sizeof(in), outData, outData + out.size());
      return out;
    }

    const le_uint32_t Value;
  };

  struct Header
  {
    Header(uint32_t crc, std::size_t origSize, std::size_t packedSize)
      : Crc(crc)
      , Original(origSize)
      , Packed(packedSize)
    {}

    static Header Decode(const TxtHeader& in)
    {
      RawHeader out;
      const auto* const inData = in.data();
      auto* const outData = out.Signature.data();
      Binary::Base64::Decode(inData, inData + in.size(), outData, outData + sizeof(out));
      Require(out.Signature == SIGNATURE);
      return {out.Crc, out.OriginalSize, out.PackedSize};
    }

    void ToRaw(RawHeader& res) const
    {
      res.Signature = SIGNATURE;
      res.Crc = Crc;
      res.OriginalSize = Original;
      res.PackedSize = Packed;
    }

    const uint32_t Crc;
    const std::size_t Original;
    const std::size_t Packed;
  };

  // clang-format off

  /* PackedBlock is 0x78 0xda always (maximal compression, no dictionary)

  bin bits: C- crc, O- original size, P- packed size, S- signature, D- data

  pos: SSSSSS SSSSSS SSSSCC CCCCCC  CCCCCC CCCCCC CCCCCC CCCCCC  OOOOOO OOOOOO OOOOOO OOOOOO  PPPPPP PPPPPP PPPPPP PPPPPP  DDDDDD DDDDDD DDDDdd
  val: 011001 000011 VVVVxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  011110 001101 1010xx
  sym: Z      D      ?      ?       ?      ?      ?      ?       ?      ?      ?      ?       ?      ?      ?      ?       e      N      opqr
  */

  // clang-format on

  static_assert(sizeof(RawMarker) * alignof(RawMarker) == 6, "Invalid layout of RawMarker");
  static_assert(sizeof(RawHeader) * alignof(RawHeader) == 12, "Invalid layout of RawHeader");

  struct Layout
  {
    Layout(const uint8_t* start, const uint8_t* end)
      : Start(start)
      , End(end)
    {}

    const TxtHeader& GetHeader() const
    {
      Require(Start + sizeof(TxtHeader) < End);
      return *safe_ptr_cast<const TxtHeader*>(Start);
    }

    StringView GetBody(std::size_t binarySize) const
    {
      const auto* bodyStart = Start + sizeof(TxtHeader);
      const auto bodySize = Binary::Base64::CalculateConvertedSize(binarySize);
      Require(bodyStart + bodySize <= End);
      return {safe_ptr_cast<const char*>(bodyStart), bodySize};
    }

  private:
    const uint8_t* const Start;
    const uint8_t* const End;
  };

  Layout FindLayout(Binary::View raw, const Marker& marker)
  {
    const auto* const rawStart = static_cast<const uint8_t*>(raw.Start());
    const uint8_t* const rawEnd = rawStart + raw.Size();
    const TxtMarker lookup = marker.Encode();
    const uint8_t* const res = std::search(rawStart, rawEnd, lookup.begin(), lookup.end());
    return {res, rawEnd};
  }

  Binary::Container::Ptr Decode(Binary::View raw, const Marker& marker)
  {
    try
    {
      const Layout layout = FindLayout(raw, marker);
      const Header hdr = Header::Decode(layout.GetHeader());
      Dbg("Found container id={}", hdr.Crc);
      const auto decoded = Binary::Base64::Decode(layout.GetBody(hdr.Packed));
      Dbg("Unpack {} => {}", hdr.Packed, hdr.Original);
      auto unpacked = Binary::Compression::Zlib::Decompress(decoded, hdr.Original);
      Require(hdr.Original == unpacked->Size());
      Require(hdr.Crc == Binary::Crc32(*unpacked));
      return unpacked;
    }
    catch (const std::exception&)
    {
      Dbg("Failed to decode");
      return {};
    }
    catch (const Error& e)
    {
      Dbg("Error: {}", e.ToString());
      return {};
    }
  }

  Header Compress(Binary::View input, Binary::DataBuilder& output)
  {
    const auto inSize = input.Size();
    const std::size_t prevOutputSize = output.Size();
    {
      Binary::DataInputStream inputStream(input);
      Binary::Compression::Zlib::Compress(inputStream, output);
    }
    const auto packedSize = output.Size() - prevOutputSize;
    return {Binary::Crc32(input), inSize, packedSize};
  }

  Binary::Container::Ptr Convert(Binary::View input)
  {
    const auto outSize = Binary::Base64::CalculateConvertedSize(input.Size());
    Binary::DataBuilder builder(outSize);
    const auto* in = input.As<uint8_t>();
    auto* out = static_cast<char*>(builder.Allocate(outSize));
    Binary::Base64::Encode(in, in + input.Size(), out, out + outSize);
    return builder.CaptureResult();
  }
}  // namespace ZXTune::Zdata

namespace ZXTune::Zdata
{
  const auto ID = "ZDATA"_id;
  const auto INFO = "Zdata"sv;
  const uint_t CAPS = Capabilities::Category::CONTAINER | Capabilities::Container::Type::ARCHIVE;
}  // namespace ZXTune::Zdata

namespace ZXTune
{
  DataLocation::Ptr BuildZdataContainer(Binary::View input)
  {
    Binary::DataBuilder builder(input.Size());
    builder.Add<Zdata::RawHeader>();
    const Zdata::Header hdr = Zdata::Compress(input, builder);
    hdr.ToRaw(builder.Get<Zdata::RawHeader>(0));
    auto data = Zdata::Convert(builder.GetView());
    return CreateLocation(std::move(data), String{Zdata::ID},
                          Strings::PrefixedIndex::Create(Zdata::PLUGIN_PREFIX, hdr.Crc).ToString());
  }
}  // namespace ZXTune

namespace ZXTune::Zdata
{
  class Plugin : public ArchivePlugin
  {
  public:
    Plugin() = default;

    PluginId Id() const override
    {
      return ID;
    }

    StringView Description() const override
    {
      return INFO;
    }

    uint_t Capabilities() const override
    {
      return CAPS;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return {};
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& /*params*/, DataLocation::Ptr input,
                                 ArchiveCallback& /*callback*/) const override
    {
      return Analysis::CreateUnmatchedResult(input->GetData()->Size());
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr location,
                              const Analysis::Path& inPath) const override
    {
      const auto& pathComp = inPath.Elements().front();
      const auto pathIndex = Strings::PrefixedIndex::Parse(PLUGIN_PREFIX, pathComp);
      if (pathIndex.IsValid())
      {
        const auto rawData = location->GetData();
        if (auto decoded = Zdata::Decode(*rawData, Zdata::Marker(static_cast<uint32_t>(pathIndex.GetIndex()))))
        {
          return CreateNestedLocation(std::move(location), std::move(decoded), ID, pathComp);
        }
      }
      return {};
    }
  };
}  // namespace ZXTune::Zdata

namespace ZXTune
{
  void RegisterZdataContainer(ArchivePluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin = MakePtr<Zdata::Plugin>();
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
