/**
* 
* @file
*
* @brief  Zdata plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/plugins_types.h"
#include "core/src/location.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <binary/base64.h>
#include <binary/crc.h>
#include <binary/compression/zlib.h>
#include <binary/compression/zlib_stream.h>
#include <binary/data_builder.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <strings/prefixed_index.h>
//std includes
#include <algorithm>
//text includes
#include <core/text/plugins.h>

namespace ZXTune
{
namespace Zdata
{
  const Debug::Stream Dbg("Core::ZData");

  typedef std::array<uint8_t, 2> SignatureType;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct UInt24LE
  {
  public:
    UInt24LE()
      : Data()
    {
    }

    /*explicit*/UInt24LE(uint_t val)
    {
      Require(0 == (val >> 24));
      Data[0] = val & 255;
      Data[1] = (val >> 8) & 255;
      Data[2] = val >> 16;
    }

    operator uint_t () const
    {
      return (Data[2] << 16) | (Data[1] << 8) | Data[0];
    }
  private:
    std::array<uint8_t, 3> Data;
  } PACK_POST;

  //4 LSBs of signature may be version
  const SignatureType SIGNATURE = {{0x64, 0x30}};

  PACK_PRE struct RawMarker
  {
    SignatureType Signature;
    uint32_t Crc;
  } PACK_POST;

  //size of data block, without header
  PACK_PRE struct RawHeader : RawMarker
  {
    UInt24LE OriginalSize;
    UInt24LE PackedSize;
  } PACK_POST;

  typedef std::array<char, 8> TxtMarker;
  typedef std::array<char, 16> TxtHeader;
  
  struct Marker
  {
    explicit Marker(uint32_t crc)
      : Value(crc)
    {
    }

    TxtMarker Encode() const
    {
      const RawMarker in = {SIGNATURE, fromLE(Value)};
      const auto inData = in.Signature.data();
      TxtMarker out;
      const auto outData = out.data();
      Binary::Base64::Encode(inData, inData + sizeof(in), outData, outData + out.size());
      return out;
    }

    const uint32_t Value;
  };

  struct Header
  {
    Header(uint32_t crc, std::size_t origSize, std::size_t packedSize)
      : Crc(crc)
      , Original(origSize)
      , Packed(packedSize)
    {
    }

    static Header Decode(const TxtHeader& in)
    {
      RawHeader out;
      const auto inData = in.data();
      const auto outData = out.Signature.data();
      Binary::Base64::Decode(inData, inData + in.size(), outData, outData + sizeof(out));
      Require(out.Signature == SIGNATURE);
      return Header(fromLE(out.Crc), out.OriginalSize, out.PackedSize);
    }

    void ToRaw(RawHeader& res) const
    {
      res.Signature = SIGNATURE;
      res.Crc = fromLE(Crc);
      res.OriginalSize = Original;
      res.PackedSize = Packed;
    }

    const uint32_t Crc;
    const std::size_t Original;
    const std::size_t Packed;
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  //PackedBlock is 0x78 0xda always (maximal compression, no dictionary)

  /*
  bin bits: C- crc, O- original size, P- packed size, S- signature, D- data
  
  pos: SSSSSS SSSSSS SSSSCC CCCCCC  CCCCCC CCCCCC CCCCCC CCCCCC  OOOOOO OOOOOO OOOOOO OOOOOO  PPPPPP PPPPPP PPPPPP PPPPPP  DDDDDD DDDDDD DDDDdd
  val: 011001 000011 VVVVxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  xxxxxx xxxxxx xxxxxx xxxxxx  011110 001101 1010xx
  sym: Z      D      ?      ?       ?      ?      ?      ?       ?      ?      ?      ?       ?      ?      ?      ?       e      N      opqr
  */

  static_assert(sizeof(RawMarker) == 6, "Invalid layout of RawMarker");
  static_assert(sizeof(RawHeader) == 12, "Invalid layout of RawHeader");

  struct Layout
  {
    Layout(const uint8_t* start, const uint8_t* end)
      : Start(start)
      , End(end)
    {
    }

    const TxtHeader& GetHeader() const
    {
      Require(Start + sizeof(TxtHeader) < End);
      return *safe_ptr_cast<const TxtHeader*>(Start);
    }

    const char* GetBody() const
    {
      return safe_ptr_cast<const char*>(Start + sizeof(TxtHeader));
    }

    const char* GetBodyEnd() const
    {
      return safe_ptr_cast<const char*>(End);
    }
  private:
    const uint8_t* const Start;
    const uint8_t* const End;
  };
  
  Layout FindLayout(Binary::View raw, const Marker& marker)
  {
    const uint8_t* const rawStart = static_cast<const uint8_t*>(raw.Start());
    const uint8_t* const rawEnd = rawStart + raw.Size();
    const TxtMarker lookup = marker.Encode();
    const uint8_t* const res = std::search(rawStart, rawEnd, lookup.begin(), lookup.end());
    return Layout(res, rawEnd);
  }

  Binary::Container::Ptr Decode(Binary::View raw, const Marker& marker)
  {
    try
    {
      const Layout layout = FindLayout(raw, marker);
      const Header hdr = Header::Decode(layout.GetHeader());
      Dbg("Found container id=%1%", hdr.Crc);
      Dump decoded(hdr.Packed);
      Binary::Base64::Decode(layout.GetBody(), layout.GetBodyEnd(), decoded.data(), decoded.data() + hdr.Packed);
      std::unique_ptr<Dump> unpacked(new Dump(hdr.Original));
      Dbg("Unpack %1% => %2%", hdr.Packed, hdr.Original);
      //TODO: use another function
      Require(hdr.Original == Binary::Compression::Zlib::Decompress(decoded, unpacked->data(), unpacked->size()));
      Require(hdr.Crc == Binary::Crc32(*unpacked));
      return Binary::CreateContainer(std::move(unpacked));
    }
    catch (const std::exception&)
    {
      Dbg("Failed to decode");
      return Binary::Container::Ptr();
    }
    catch (const Error& e)
    {
      Dbg("Error: %1%", e.ToString());
      return Binary::Container::Ptr();
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
    return Header(Binary::Crc32(input), inSize, packedSize);
  }

  Binary::Container::Ptr Convert(const void* input, std::size_t inputSize)
  {
    const std::size_t outSize = Binary::Base64::CalculateConvertedSize(inputSize);
    std::unique_ptr<Dump> result(new Dump(outSize));
    const uint8_t* const in = static_cast<const uint8_t*>(input);
    char* const out = safe_ptr_cast<char*>(result->data());
    Binary::Base64::Encode(in, in + inputSize, out, out + outSize);
    return Binary::CreateContainer(std::move(result));
  }
}
}

namespace ZXTune
{
namespace Zdata
{
  const Char ID[] = {'Z', 'D', 'A', 'T', 'A', 0};
  const Char* const INFO = Text::ZDATA_PLUGIN_INFO;
  const uint_t CAPS = Capabilities::Category::CONTAINER | Capabilities::Container::Type::ARCHIVE;
}
}

namespace ZXTune
{
  DataLocation::Ptr BuildZdataContainer(Binary::View input)
  {
    Binary::DataBuilder builder(input.Size());
    builder.Add<Zdata::RawHeader>();
    const Zdata::Header hdr = Zdata::Compress(input, builder);
    hdr.ToRaw(builder.Get<Zdata::RawHeader>(0));
    const Binary::Container::Ptr data = Zdata::Convert(builder.Get(0), builder.Size());
    return CreateLocation(data, Zdata::ID, Strings::PrefixedIndex(Text::ZDATA_PLUGIN_PREFIX, hdr.Crc).ToString());
  }
}

namespace ZXTune
{
namespace Zdata
{
  class Plugin : public ArchivePlugin
  {
  public:
    Plugin()
      : Description(CreatePluginDescription(ID, INFO, CAPS))
    {
    }

    ZXTune::Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Binary::Format::Ptr();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& /*params*/, DataLocation::Ptr input, Module::DetectCallback& /*callback*/) const override
    {
      return Analysis::CreateUnmatchedResult(input->GetData()->Size());
    }

    DataLocation::Ptr Open(const Parameters::Accessor& /*params*/, DataLocation::Ptr location, const Analysis::Path& inPath) const override
    {
      const String& pathComp = inPath.GetIterator()->Get();
      const Strings::PrefixedIndex pathIndex(Text::ZDATA_PLUGIN_PREFIX, pathComp);
      if (pathIndex.IsValid())
      {
        const Binary::Data::Ptr rawData = location->GetData();
        if (const Binary::Container::Ptr decoded = Zdata::Decode(*rawData, Zdata::Marker(static_cast<uint32_t>(pathIndex.GetIndex()))))
        {
          return CreateNestedLocation(location, decoded, ID, pathComp);
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const ZXTune::Plugin::Ptr Description;
  };
}

  void RegisterZdataContainer(ArchivePluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin = MakePtr<Zdata::Plugin>();
    registrator.RegisterPlugin(plugin);
  }
}
