/*
Abstract:
  DSQ implicit convertors support

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
#include <detector.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <algorithm>
#include <iterator>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char DSQ_PLUGIN_ID[] = {'D', 'S', 'Q', '\0'};
  const String DSQ_PLUGIN_VERSION(FromStdString("$Rev$"));

  /*
     classic depacker
     bitstream:
     %0,b8,continue
     %11,len=2+b1
     %101,len=4+b2
     %1001,len=8+b4 if len==0x17 copy 0xe+b5 bytes,continue
     %1000,len=0x17+b8 untill data!=0xff
     %1,offset=0x221+bX
     %01,offset=1+b5
     %00,offset=0x21+b9
     backcopy len bytes from offset,continue
  */
  const std::string DSQ_DEPACKER_PATTERN(
    "11??"      // ld de,xxxx ;buffer
    "21??"      // ld hl,xxxx ;addr+start depacker
    "d5"        // push de
    "01??"      // ld bc,xxxx ;rest depacker size
    "edb0"      // ldir
    "11??"      // ld de,xxxx ;addr
    "21??"      // ld hl,xxxx ;packed data start
//packed size. word parameter. offset=0x13
    "01??"      // ld bc,xxxx ;packed size
    "c9"        // ret
    "edb0"      // ldir
    "010801"    // ld bc,#108 ;b- counter of bits, c=8
    "21??"      // ld hl,xxxx ;last packed byte
    "d9"        // exx
    "e5"        // push hl
//last depacked. word parameter. offset=0x21
    "11??"      // ld de,xxxx ;last depacked byte
                //depcycle:
    "210100"    // ld hl,1    ;l- bytes to copy
    "cd??"      // call getbit
    "3027"      // jr nc,copy_byte
    "45"        // ld b,l     ;%1
    "2c"        // inc l
    "cd??"      // call getbit
    "3830"      // jr c, +62
    "04"        // inc b       ;%10
    "2e04"      // ld l,4
    "cd??"      // call getbit
    "3828"      // jr c, +62
    "cd??"      // call getbit ;%100
    "301f"      // jr nc, +5e
    "45"        // ld b,l      ;%1001
    "cd??"      // call getbits
    "c608"      // ld a,8
    "6f"        // ld l,a
    "fe17"      // cp #17      ;?
    "381f"      // jr c, +69
    "0605"       // ld b,5      ;0xe+b5
    "cd??"      // call getbits
    "c60e"      // ld a,14
    "6f"        // ld l,a
                //copy_byte:
    "0608"      // ld b,8
    "cd??"      // call getbits
    "12"        // ld (de),a
    "1b"        // dec de
    "2d"        // dec l
    "20f6"      // jr nz,copy_byte
    "182f"      // jr  +8d
    "2e17"      // ld l,#17
    "0608"      // ld b,8       ;8bits offset
    "cd??"      // call getbits
    "09"        // add hl,bc
    "3c"        // inc a
    "28f7"      // jr z, +60    ;repeat while offset==0xff
    "e5"        // push hl
    "cd??"      // call getbit
    "212100"    // ld hl,#21
    "380f"      // jr c,xx
    "0609"      // ld b,9
    "19"        // add hl,de
    "cd??"      // call getbit
    "300c"      // jr nc,do_getbits
    "0605"      // ld b,5
    "6b"        // ld l,e
    "62"        // ld h,d
    "23"        // inc hl
    "1805"      // jr do_getbits
//byte parameter. offset=0x82
    "06?"       // ld b,xx
    "2602"      // ld h,2
    "19"        // add hl,de
                //do_getbits:
    "cd??"      // call getbits
    "09"        // add hl,bc
    "c1"        // pop bc
    "edb8"      // lddr
//word parameter. offset=0x8e
    "21??"      // ld hl,xxxx ;depack start-1
    "ed52"      // sbc hl,de
    "388f"      // jr c,depcycle
    "e1"        // pop hl
    "d9"        // exx
  );
/*
+96  c3 92 9c	jp xxxx  ;jump to addr

;0xbf83 getting bits set, b- count
getbits to ba(bc)
+99  af		xor a
+9a  4f		ld c,a
+9b  cd 90 bf	call getbit
+9e  cb 11	rl c
+a0  17		rla         
+a1  10 f8	djnz +9b
+a3  47		ld b,a
+a4  79		ld a,c
+a5  c9		ret

;0xbf90 getting bit
getbit:
+a6  d9		exx
+a7  10 03	djnz +ac
+a9  41		ld b,c
+aa  5e		ld e,(hl)
+ab  2b		dec hl
+ac  cb 13	rl e
+ae  d9		exx
+af  c9		ret
*/

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct DSQDepacker
  {
    uint8_t Padding1[0x13];
    //+0x13
    uint16_t PackedSize;
    //+0x15
    uint8_t Padding2[0x0c];
    //+0x21
    uint16_t DstRbegin;
    //+0x23
    uint8_t Padding3[0x5f];
    //+0x82
    uint8_t LongOffsetBits;
    //+0x83
    uint8_t Padding4[0xb];
    //+0x8e
    uint16_t DstRend;
    //+0x90
    uint8_t Padding5[0x20];

    uint_t GetPackedSize() const
    {
      return fromLE(PackedSize);
    }

    uint_t GetDepackedSize() const
    {
      return fromLE(DstRbegin) - fromLE(DstRend);
    }

    bool Check(uint_t limit) const;
    bool Decode(Dump& dst) const;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(DSQDepacker) == 0xb0);

  //dsq bitstream decoder
  class Bitstream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : Data(data), Pos(Data + size), Bits(), Mask(0)
    {
    }

    bool Eof() const
    {
      return Pos < Data;
    }

    uint8_t GetByte()
    {
      return Eof() ? 0 : *--Pos;
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
    const uint8_t* const Data;
    const uint8_t* Pos;
    uint_t Bits;
    uint_t Mask;
  };

  class BitstreamHelper
  {
  public:
    BitstreamHelper(Bitstream& stream, unsigned longOffsetBits)
      : Stream(stream)
      , LongOffsetBits(longOffsetBits)
    {
    }

    unsigned GetData()
    {
      return Stream.GetBits(8);
    }

    unsigned GetLength()
    {
      //%1
      if (Stream.GetBit())
      {
        //%11
        return 2 + Stream.GetBit();
      }
      //%10
      if (Stream.GetBit())
      {
        //%101
        return 4 + Stream.GetBits(2);
      }
      //%100
      if (Stream.GetBit())
      {
        //%1001
        return 8 + Stream.GetBits(4);
      }
      //%1000
      unsigned res = 0x17;
      for (unsigned add = 0xff; add == 0xff;)
      {
        add = Stream.GetBits(8);
        res += add;
      }
      return res;
    }

    unsigned GetOffset()
    {
      if (Stream.GetBit())
      {
        //%1
        return 0x221 + Stream.GetBits(LongOffsetBits);
      }
      //%0
      if (Stream.GetBit())
      {
        //%01
        return 1 + Stream.GetBits(5);
      }
      //%00
      return 0x21 + Stream.GetBits(9);
    }
  private:
    Bitstream& Stream;
    const unsigned LongOffsetBits;
  };

  inline bool CopyFromBack(int_t offset, Dump& dst, uint_t count)
  {
    assert(offset <= 0);
    const std::size_t size = dst.size();
    if (uint_t(-offset) > size)
    {
      return false;//invalid backref
    }
    dst.resize(size + count);
    const Dump::iterator dstStart = dst.begin() + size;
    const Dump::const_iterator srcStart = dstStart + offset;
    const Dump::const_iterator srcEnd = srcStart + count;
    RecursiveCopy(srcStart, srcEnd, dstStart);
    return true;
  }

  bool DSQDepacker::Check(uint_t limit) const
  {
    if ((GetPackedSize() + sizeof(*this) > limit) ||
        (fromLE(DstRend) >= fromLE(DstRbegin)) ||
        !DetectFormat(Padding1, sizeof(*this), DSQ_DEPACKER_PATTERN))
    {
      return false;
    }
    return true;
  }

  bool DSQDepacker::Decode(Dump& res) const
  {
    const uint_t packedSize = GetPackedSize();
    const uint_t unpackedSize = GetDepackedSize();
    Dump reverse;
    reverse.reserve(unpackedSize);

    std::back_insert_iterator<Dump> target(reverse);
    Bitstream stream(Padding1 + sizeof(*this), packedSize);
    BitstreamHelper helper(stream, LongOffsetBits);
    while (!stream.Eof() &&
           reverse.size() < unpackedSize)
    {
      //%0
      if (!stream.GetBit())
      {
        *target = helper.GetData();
        ++target;
      }
      else
      {
        const unsigned len = helper.GetLength();
        if (len == 0x17)
        {
          const unsigned count = 14 + stream.GetBits(5);
          std::generate_n(target, count, boost::bind(&BitstreamHelper::GetData, &helper));
        }
        else
        {
          const uint_t offset = helper.GetOffset();
          if (offset > reverse.size())
          {
            return false;
          }
          CopyFromBack(-static_cast<int_t>(offset), reverse, len);
        }
      }
    }
    std::reverse(reverse.begin(), reverse.end());
    res.swap(reverse);
    return true;
  }

  class DSQPlugin : public ImplicitPlugin
  {
  public:
    virtual String Id() const
    {
      return DSQ_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::DSQ_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return DSQ_PLUGIN_VERSION;
    }
    
    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t limit = inputData.Size();
      if (limit < sizeof(DSQDepacker))
      {
        return false;
      }
      const DSQDepacker* const depacker = safe_ptr_cast<const DSQDepacker*>(data);
      return depacker->Check(limit);
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Map& /*commonParams*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const DSQDepacker* const depacker = safe_ptr_cast<const DSQDepacker*>(data);
      assert(depacker->Check(inputData.Size()));

      Dump res;
      if (depacker->Decode(res))
      {
        region.Offset = 0;
        region.Size = depacker->GetPackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterDSQConvertor(PluginsEnumerator& enumerator)
  {
    const ImplicitPlugin::Ptr plugin(new DSQPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
