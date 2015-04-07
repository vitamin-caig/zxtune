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
#include "container_supp_common.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/src/location.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <error.h>
//library includes
#include <binary/base64.h>
#include <binary/compress.h>
#include <binary/data_builder.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
//text includes
#include <core/text/plugins.h>

namespace
{
  const Debug::Stream Dbg("Core::ZData");
}

namespace
{
  typedef boost::array<uint8_t, 2> SignatureType;

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
    boost::array<uint8_t, 3> Data;
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

  typedef boost::array<char, 8> TxtMarker;
  typedef boost::array<char, 16> TxtHeader;

  struct Marker
  {
    explicit Marker(uint32_t crc)
      : Value(crc)
    {
    }

    TxtMarker Encode() const
    {
      const RawMarker in = {SIGNATURE, fromLE(Value)};
      TxtMarker out;
      Binary::Base64::Encode(safe_ptr_cast<const uint8_t*>(&in), safe_ptr_cast<const uint8_t*>(&in + 1), out.begin(), out.end());
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
      Binary::Base64::Decode(in.begin(), in.end(), safe_ptr_cast<uint8_t*>(&out), safe_ptr_cast<uint8_t*>(&out + 1));
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

  BOOST_STATIC_ASSERT(sizeof(RawMarker) == 6);
  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 12);

  const IndexPathComponent ZdataPath(Text::ZDATA_PLUGIN_PREFIX);

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
  
  Layout FindLayout(const Binary::Data& raw, const Marker& marker)
  {
    const uint8_t* const rawStart = static_cast<const uint8_t*>(raw.Start());
    const uint8_t* const rawEnd = rawStart + raw.Size();
    const TxtMarker lookup = marker.Encode();
    const uint8_t* const res = std::search(rawStart, rawEnd, lookup.begin(), lookup.end());
    return Layout(res, rawEnd);
  }

  Binary::Container::Ptr Decode(const Binary::Data& raw, const Marker& marker)
  {
    try
    {
      const Layout layout = FindLayout(raw, marker);
      const Header hdr = Header::Decode(layout.GetHeader());
      Dbg("Found container id=%1%", hdr.Crc);
      Dump decoded(hdr.Packed);
      Binary::Base64::Decode(layout.GetBody(), layout.GetBodyEnd(), &decoded[0], &decoded[0] + hdr.Packed);
      std::auto_ptr<Dump> unpacked(new Dump(hdr.Original));
      Dbg("Unpack %1% => %2%", hdr.Packed, hdr.Original);
      Binary::Compression::Zlib::Decompress(decoded, *unpacked);
      Require(hdr.Crc == Crc32(&unpacked->front(), unpacked->size()));
      return Binary::CreateContainer(unpacked);
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

  Header Compress(const Binary::Data& input, Binary::DataBuilder& output)
  {
    const std::size_t outSizeIn = output.Size();
    const std::size_t inSize = input.Size();
    const std::size_t inPackedSizeMax = Binary::Compression::Zlib::CalculateCompressedSizeUpperBound(inSize);
    const uint8_t* const in = static_cast<const uint8_t*>(input.Start());
    uint8_t* const outBegin = static_cast<uint8_t*>(output.Allocate(inPackedSizeMax));
    uint8_t* const outEnd = Binary::Compression::Zlib::Compress(in, in + inSize, outBegin, outBegin + inPackedSizeMax);
    const std::size_t packedSize = (outEnd - outBegin);
    output.Resize(outSizeIn + packedSize);
    return Header(Crc32(in, inSize), inSize, packedSize);
  }

  Binary::Container::Ptr Convert(const void* input, std::size_t inputSize)
  {
    const std::size_t outSize = Binary::Base64::CalculateConvertedSize(inputSize);
    std::auto_ptr<Dump> result(new Dump(outSize));
    const uint8_t* const in = static_cast<const uint8_t*>(input);
    char* const out = safe_ptr_cast<char*>(&result->front());
    Binary::Base64::Encode(in, in + inputSize, out, out + outSize);
    return Binary::CreateContainer(result);
  }
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'Z', 'D', 'A', 'T', 'A', 0};
  const Char* const INFO = Text::ZDATA_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;
}

namespace ZXTune
{
  DataLocation::Ptr BuildZdataContainer(const Binary::Data& input)
  {
    Binary::DataBuilder builder(input.Size());
    builder.Add<RawHeader>();
    const Header hdr = Compress(input, builder);
    hdr.ToRaw(builder.Get<RawHeader>(0));
    const Binary::Container::Ptr data = Convert(builder.Get(0), builder.Size());
    return CreateLocation(data, ID, ZdataPath.Build(hdr.Crc));
  }
}

namespace
{
  class ZdataPlugin : public ArchivePlugin
  {
  public:
    ZdataPlugin()
      : Description(CreatePluginDescription(ID, INFO, CAPS))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Binary::Format::Ptr();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& /*callback*/) const
    {
      return Analysis::CreateUnmatchedResult(input->GetData()->Size());
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const Analysis::Path& inPath) const
    {
      const String& pathComp = inPath.GetIterator()->Get();
      Parameters::IntType marker;
      if (ZdataPath.GetIndex(pathComp, marker))
      {
        const Binary::Data::Ptr rawData = location->GetData();
        if (const Binary::Container::Ptr decoded = Decode(*rawData, Marker(static_cast<uint32_t>(marker))))
        {
          return CreateNestedLocation(location, decoded, ID, pathComp);
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
  };
}

namespace ZXTune
{
  void RegisterZdataContainer(ArchivePluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new ZdataPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
