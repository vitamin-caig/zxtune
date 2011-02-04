/*
Abstract:
  Trush convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "hrust1_bitstream.h"
#include "pack_utils.h"
#include <core/plugins/enumerator.h>
//common includes
#include <byteorder.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <algorithm>
#include <iterator>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char TRUSH_PLUGIN_ID[] = {'T', 'R', 'U', 'S', 'H', '\0'};
  const String TRUSH_PLUGIN_VERSION(FromStdString("$Rev$"));

  const char TRUSH_SIGNATURE[] =
  {
    'C', 'O', 'M', 'P', 'R', 'E', 'S', 'S', 'O', 'R', ' ', 'B', 'Y', ' ',
    'A', 'L', 'E', 'X', 'A', 'N', 'D', 'E', 'R', ' ', 'T', 'R', 'U', 'S', 'H', ' ', 'O', 'D', 'E', 'S', 'S', 'A'
  };

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct TRUSHHeader
  {
    //+0
    char Padding1[0x0c];
    //+c
    uint16_t SizeOfPacked;
    //+e
    char Padding2[0x19];
    //+27
    char Signature[0x24];
    //+4b
    char Padding3[0xce];
    //+119
    uint8_t Bitstream[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(TRUSHHeader) == 0x11b);

  typedef Hrust1Bitstream Bitstream;

  uint_t GetPackedSize(const TRUSHHeader& header)
  {
    return sizeof(header) + fromLE(header.SizeOfPacked) - sizeof(header.Bitstream);
  }

  bool CheckTRUSH(const TRUSHHeader* header, std::size_t limit)
  {
    BOOST_STATIC_ASSERT(sizeof(header->Signature) == sizeof(TRUSH_SIGNATURE));
    if (limit < sizeof(*header) ||
        0 != std::memcmp(header->Signature, TRUSH_SIGNATURE, sizeof(header->Signature)))
    {
      return false;
    }
    const uint_t packed = GetPackedSize(*header);
    return packed <= limit &&
      0xff == header->Bitstream[fromLE(header->SizeOfPacked) - 1];
  }

  uint_t GetOffset(Bitstream& stream)
  {
    if (stream.GetBit())
    {
      return stream.GetByte() + 1;
    }
    const uint_t code1 = stream.GetBits(3);
    if (code1 < 2)
    {
      return 256 * (code1 + 1) + stream.GetByte() + 1;
    }
    const uint_t code2 = 2 * code1 + stream.GetBit();
    if (code2 < 8)
    {
      return 256 * (code2 - 1) + stream.GetByte() + 1;
    }
    const uint_t code3 = 2 * code2 + stream.GetBit();
    if (code3 < 0x17)
    {
      return 256 * (code3 - 9) + stream.GetByte() + 1;
    }
    const uint_t code4 = (2 * code3 + stream.GetBit()) & 0x1f;
    const uint_t lodisp = stream.GetByte();
    return 256 * code4 + lodisp + 1;
  }

  bool DecodeTRUSH(const TRUSHHeader& header, Dump& res)
  {
    const uint_t packedSize = fromLE(header.SizeOfPacked);
    Dump dst;
    dst.reserve(packedSize * 2);//TODO

    Bitstream stream(header.Bitstream, packedSize);
    while (!stream.Eof())
    {
      //%0 - put byte
      if (!stream.GetBit())
      {
        dst.push_back(stream.GetByte());
        continue;
      }
      uint_t code = stream.GetBits(2);
      if (code)
      {
        code = 2 * code + stream.GetBit();
        if (code >= 6)
        {
          code = 2 * code + stream.GetBit();
        }
      }
      if (2 == code)
      {
        const uint_t offset = stream.GetByte() + 1;
        if (!CopyFromBack(offset, dst, 2))
        {
          return false;
        }
        continue;
      }
      uint_t len = code < 2 ? code + 3 : 0;
      if (3 == code)
      {
        const uint_t data = stream.GetByte();
        if (0xff == data)
        {
          break;
        }
        else if (0xfe == data)
        {
          len = stream.GetLEWord();
        }
        else
        {
          len = data + 10;
        }
      }
      else if (code > 3)
      {
        len = code;
        if (len >= 6)
        {
          len -= 6;
        }
      }
      const uint_t offset = GetOffset(stream);
      if (!CopyFromBack(offset, dst, len))
      {
        return false;
      }
    }
    res.swap(dst);
    return true;
  }

  class TRUSHData
  {
  public:
    TRUSHData(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const TRUSHHeader* const header = safe_ptr_cast<const TRUSHHeader*>(data);
      if (CheckTRUSH(header, size))
      {
        Header = header;
      }
    }

    bool IsValid() const
    {
      return Header != 0;
    }

    uint_t PackedSize() const
    {
      assert(Header);
      return GetPackedSize(*Header);
    }

    bool Decode(Dump& res) const
    {
      assert(Header);
      return DecodeTRUSH(*Header, res);
    }
  private:
    const TRUSHHeader* Header;
  };

  class TRUSHPlugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return TRUSH_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::TRUSH_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return TRUSH_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const TRUSHData trushData(data, size);
      return trushData.IsValid();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const IO::DataContainer& data, ModuleRegion& region) const
    {
      const TRUSHData trushData(static_cast<const uint8_t*>(data.Data()), data.Size());
      assert(trushData.IsValid());
      Dump res;
      if (trushData.Decode(res))
      {
        region.Offset = 0;
        region.Size = trushData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterTRUSHConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new TRUSHPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
