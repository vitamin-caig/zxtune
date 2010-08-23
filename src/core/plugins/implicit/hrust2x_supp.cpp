/*
Abstract:
  Hrust 2.x implicit convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <numeric>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char HRUST2X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '2', '\0'};
  const String HRUST2X_PLUGIN_VERSION(FromStdString("$Rev$"));

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Hrust21Header
  {
    uint8_t ID[3];//'hr2'
    uint8_t Flag;//'1' | 128
    uint16_t DataSize;
    uint16_t PackedSize;
    Hrust2xHeader PackedData;

    //flag bits
    enum
    {
      NO_COMPRESSION = 128
    };
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  //hrust2x bitstream decoder
  class Bitstream
  {
  public:
    Bitstream(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data)), End(Data + size), Bits(), Mask(0)
    {
    }

    bool Eof() const
    {
      return Data >= End;
    }

    uint8_t GetByte()
    {
      return Eof() ? 0 : *Data++;
    }

    uint_t GetBit()
    {
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Mask = 0x80;
      }
      return Bits & Mask ? 1 : 0;
    }

    uint_t GetBits(unsigned count)
    {
      uint_t result = 0;
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }
  private:
    const uint8_t* Data;
    const uint8_t* const End;
    uint_t Bits;
    uint_t Mask;
  };

  inline int_t GetDist(Bitstream& stream)
  {
    //%1,disp8
    if (stream.GetBit())
    {
      return static_cast<int16_t>(0xff00 + stream.GetByte());
    }
    else
    {
      //%011x,%010xx,%001xxx,%000xxxx,%0000000
      uint_t res = 0xffff;
      for (uint_t bits = 4 - stream.GetBits(2); bits; --bits)
      {
        res = (res << 1) + stream.GetBit() - 1;
      }
      res &= 0xffff;
      if (0xffe1 == res)
      {
        res = stream.GetByte();
      }
      return static_cast<int16_t>((res << 8) + stream.GetByte());
    }
  }

  inline bool CopyFromBack(int_t offset, Dump& dst, uint_t count)
  {
    if (offset < -int_t(dst.size()))
    {
      return false;//invalid backref
    }
    std::copy(dst.end() + offset, dst.end() + offset + count, std::back_inserter(dst));
    return true;
  }

  bool Decode(const Hrust21Header* header, std::size_t size, Dump& dst)
  {
    //check for format
    if (size < sizeof(*header) ||
      header->ID[0] != 'h' || header->ID[1] != 'r' || header->ID[2] != '2' ||
      fromLE(header->PackedSize) > fromLE(header->DataSize) ||
      fromLE(header->PackedSize) > size)
    {
      return false;
    }

    if (0 != (header->Flag & header->NO_COMPRESSION))
    {
      //just copy
      dst.resize(fromLE(header->DataSize));
      std::memcpy(&dst[0], header->PackedData.LastBytes, dst.size());
      return true;
    }
    dst.reserve(fromLE(header->DataSize));
    return DecodeHrust2x(&header->PackedData, fromLE(header->PackedSize), dst) &&
      dst.size() == fromLE(header->DataSize);//valid if match
  }

  //////////////////////////////////////////////////////////////////////////
  class Hrust2xPlugin : public ImplicitPlugin
  {
  public:
    virtual String Id() const
    {
      return HRUST2X_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUST2X_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUST2X_PLUGIN_VERSION;
    }
    
    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Map& /*commonParams*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const std::size_t limit = inputData.Size();
      const Hrust21Header* const header = static_cast<const Hrust21Header*>(inputData.Data());
      Dump res;
      //only check without depacker- depackers are separate
      if (Decode(header, limit, res))
      {
        region.Offset = 0;
        region.Size = fromLE(header->PackedSize) + 8;
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

//function from global namespace- used in hrip depacker
bool DecodeHrust2x(const Hrust2xHeader* header, uint_t size, Dump& dst)
{
  //put first byte
  dst.push_back(header->FirstByte);
  //start bitstream
  Bitstream stream(header->BitStream, size);
  while (!stream.Eof())
  {
    //%1,byte
    while (stream.GetBit())
    {
      dst.push_back(stream.GetByte());
    }
    uint_t len = 1;
    for (uint_t bits = 3; bits == 0x3 && len != 0x10;)
    {
      bits = stream.GetBits(2), len += bits;
    }
    //%01100..
    if (4 == len)
    {
      //%011001
      if (stream.GetBit())
      {
        uint_t len = stream.GetByte();
        if (!len)
        {
          break;//eof
        }
        else if (len < 16)
        {
          len = len * 256 | stream.GetByte();
        }
        const int_t offset = GetDist(stream);
        if (!CopyFromBack(offset, dst, len))
        {
          return false;
        }
      }
      else//%011000xxxx
      {
        for (uint_t len = 2 * (stream.GetBits(4) + 6); len; --len)
        {
          dst.push_back(stream.GetByte());
        }
      }
    }
    else
    {
      if (len > 4)
      {
        --len;
      }
      const int_t offset = 1 == len
        ? static_cast<int16_t>(0xfff8 + stream.GetBits(3))
        : (2 == len ? static_cast<int16_t>(0xff00 + stream.GetByte()) : GetDist(stream));
      if (!CopyFromBack(offset, dst, len))
      {
        return false;
      }
    }
  }
  std::copy(header->LastBytes, ArrayEnd(header->LastBytes), std::back_inserter(dst));
  return true;
}

namespace ZXTune
{
  void RegisterHrust2xConvertor(PluginsEnumerator& enumerator)
  {
    const ImplicitPlugin::Ptr plugin(new Hrust2xPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
