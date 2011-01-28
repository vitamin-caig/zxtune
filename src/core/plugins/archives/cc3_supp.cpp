/*
Abstract:
  CodeCrunvher v3 convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
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
#include <iterator>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/plugins.h>

namespace CodeCruncher3
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    //classic depacker
    "?"       // di/nop
    "???"     // usually 'K','S','A','!'
    "21??"    // ld hl,xxxx                                        754a/88d2
    "11??"    // ld de,xxxx                                        5b81/5b81
    "017f00"  // ld bc,0x007f rest depacker size
    "d5"      // push de
    "edb0"    // ldir
    "21??"    // ld hl,xxxx last of src packed       (data = +#11) a15b/c949
    "11??"    // ld de,xxxx last of target depack    (data = +#14) ffff/ffff
    "01??"    // ld bc,xxxx size of packed           (data = +#17) 2b93/3ff9
    "c9"      // ret
    //+0x1a
    "ed?"     // lddr/ldir
    "21??"    // ld hl,xxxx src of packed after move (data = +#1d) d46d/c007
    "11??"    // ld de,xxxx dst of depack            (data = +#20) 7530/88b8
    "7e"      // ld a,(hl)
    "cb3f"    // srl a
    "382b"    // jr c,xx
    "e607"    // and 7
    "47"      // ld b,a
    "ed6f"    // rld
    "23"      // inc hl
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "e5"      // push hl
    "08"      // ex af,af'
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "62"      // ld h,d
    "6b"      // ld l,e
    "ed42"    // sbc hl,bc
    "0600"    // ld b,xx
    "3C"      // inc a
    "4f"      // ld c,a
    "03"      // inc bc
    "03"      // inc bc
    "edb0"    // ldir
    "fe10"    // cp xx
    "200b"    // jr nz,xx
    "e3"      // ld (sp),hl
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "e3"      // ld (sp),hl
    "08"      // ex af,af'
    "4f"      // ld c,a
    "b7"      // or a
    "2802"    // jr z,xx
    "edb0"    // ldir
    "08"      // ex af,af'
    "e1"      // pop hl
    "18d1"    // jr xxxx
/*
    "23"      // inc hl
    "cb3f"    // srl a
    "3823"    // jr c,xx
    "1f"      // rra
    "3813"    // jr c,xx
    "1f"      // rra
    "4f"      // ld c,a
    "3803"    // jr c,xx
    "41"      // ld b,c
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "12"      // ld (de),a
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "e5"      // push hl
    "62"      // ld h,d
    "6b"      // ld l,e
    "13"      // inc de
    "7a"      // ld a,d
    "18ce"    // jr xxxx
    "cb3f"    // srl a
    "4f"      // ld c,a
    "3003"    // jr nc,xxxx
    "41"      // ld b,c
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "03"      // inc bc
    "edb0"    // ldir
    "18a8"    // jr xxxx
    "cb3f"    // srl a
    "380a"    // jr c,xx
    "1f"      // rra
    "3014"    // jr nc,xxxx
    "3d"      // dec a
    "12"      // ld (de),a
    "13"      // inc de
    "12"      // ld (de),a
    "13"      // inc de
    "189a"    // jr xxxx
    "08"      // ex af,af'
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "e5"      // push hl
    "62"      // ld h,d
    "6b"      // ld l,e
    "2b"      // dec hl
    "3d"      // dec a
    "20fc"    // jr nz,xx
    "48"      // ld c,b
    "18a6"    // jr xxxx
    "?"       // ei/nop
    "c3??"    // jp xxxx
*/
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct FormatHeader
  {
    //+0
    uint8_t Padding1[0x17];
    //+0x17
    uint16_t SizeOfPacked;
    //+0x19
    uint8_t Padding2[0x80];
    //+0x99
    uint8_t Data[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(FormatHeader) == 0x99 + 1);

  bool IsFinishMarker(uint_t data)
  {
    return 3 == (data & 0x0f);
  }

  class Container
  {
  public:
    explicit Container(const ZXTune::IO::DataContainer& inputData)
      : Data(static_cast<const uint8_t*>(inputData.Data()))
      , Size(inputData.Size())
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(FormatHeader))
      {
        return false;
      }
      const uint_t usedSize = GetUsedSize();
      if (usedSize > Size ||
          !IsFinishMarker(Data[usedSize - 1]))
      {
        return false;
      }
      return true;
    }

    bool FullCheck() const
    {
      if (!FastCheck())
      {
        return false;
      }
      return DetectFormat(Data, Size, DEPACKER_PATTERN);
    }

    uint_t GetUsedSize() const
    {
      const FormatHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.SizeOfPacked) - sizeof(header.Data);
    }

    const FormatHeader& GetHeader() const
    {
      assert(Size >= sizeof(FormatHeader));
      return *safe_ptr_cast<const FormatHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class Decoder
  {
  public:
    explicit Decoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.Data, fromLE(Header.SizeOfPacked))
    {
    }

    const Dump* GetDecodedData()
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
      return IsValid ? &Decoded : 0;
    }
  private:
    bool DecodeData()
    {
      Decoded.reserve(2 * fromLE(Header.SizeOfPacked));
      //assume that first byte always exists due to header format
      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint_t data = Stream.GetByte();
        const bool isEnd = Stream.Eof();
        if (IsFinishMarker(data))
        {
          //exit
          //do not check if real decoded size is equal to calculated;
          //do not check if eof is reached
          return true;
        }
        else if (isEnd)
        {
          return false;
        }
        const bool res = (0 == (data & 1))
          ? ProcessBackReference(data)
          : ProcessCommand(data);
        if (!res)
        {
          return false;
        }
      }
      return false;
    }

    bool ProcessBackReference(uint_t data)
    {
      const uint_t loNibble = data & 0x0f;
      const uint_t hiNibble = (data & 0xf0) >> 4;
      const uint_t shortLen = hiNibble + 3;
      const uint_t offset = 128 * loNibble + Stream.GetByte();
      const uint_t len = 0x0f == hiNibble ? shortLen + Stream.GetByte() : shortLen;
      return CopyFromBack(offset, Decoded, len);
    }

    bool ProcessCommand(uint_t data)
    {
      const uint_t loNibble = data & 0x0f;
      const uint_t hiNibble = (data & 0xf0) >> 4;

      switch (loNibble)
      {
      case 0x01://long RLE
        {
          const uint_t len = 256 * hiNibble + Stream.GetByte() + 3;
          std::fill_n(std::back_inserter(Decoded), len, Stream.GetByte());
        }
        return true;
      //case 0x03://exit
      case 0x05://short copy
        std::generate_n(std::back_inserter(Decoded), hiNibble + 1, boost::bind(&ByteStream::GetByte, &Stream));
        return true;
      case 0x09://short RLE
        std::fill_n(std::back_inserter(Decoded), hiNibble + 3, Stream.GetByte());
        return true;
      case 0x0b://2 bytes
        std::fill_n(std::back_inserter(Decoded), 2, hiNibble - 1);
        return true;
      case 0x0d://long copy
        {
          const uint_t len = 256 * hiNibble + Stream.GetByte() + 1;
          std::generate_n(std::back_inserter(Decoded), len, boost::bind(&ByteStream::GetByte, &Stream));
        }
        return true;
      default://short backref
        return CopyFromBack((data & 0xf8) >> 3, Decoded, 2);
      }
    }
  private:
    bool IsValid;
    const FormatHeader& Header;
    ByteStream Stream;
    Dump Decoded;
  };
}

namespace
{
  using namespace ZXTune;

  const Char CC3_PLUGIN_ID[] = {'C', 'C', '3', '\0'};
  const String CC3_PLUGIN_VERSION(FromStdString("$Rev$"));

  class CC3Plugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return CC3_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::CC3_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return CC3_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const CodeCruncher3::Container container(inputData);
      return container.FullCheck();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const CodeCruncher3::Container container(inputData);
      assert(container.FullCheck());
      CodeCruncher3::Decoder decoder(container);
      if (const Dump* res = decoder.GetDecodedData())
      {
        region.Size = container.GetUsedSize();
        return IO::CreateDataContainer(*res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterCC3Convertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new CC3Plugin());
    enumerator.RegisterPlugin(plugin);
  }
}
