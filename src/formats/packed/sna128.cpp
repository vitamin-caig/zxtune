/*
Abstract:
  SNA128 support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <numeric>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/packed.h>

namespace Sna128
{
  typedef boost::array<uint8_t, 16384> PageData;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    uint8_t RegI;
    uint16_t RegHL_;
    uint16_t RegDE_;
    uint16_t RegBC_;
    uint16_t RegAF_;
    uint16_t RegHL;
    uint16_t RegDE;
    uint16_t RegBC;
    uint16_t RegIY;
    uint16_t RegIX;
    //+0x13 %000000xx
    uint8_t IFF;
    uint8_t RegR;
    uint16_t RegAF;
    uint16_t RegSP;
    //+0x19 0/1/2
    uint8_t ImMode;
    //+0x1a 0..7
    uint8_t Border;
    PageData Page5;
    PageData Page2;
    PageData ActivePage;
    uint16_t RegPC;
    uint8_t Port7FFD;
    //+0xc01e 0/1
    uint8_t TRDosROM;
    PageData Pages[5];
    //optional page starts here
  } PACK_POST;

  //5,2,0,1,3,4,6,7
  typedef boost::array<PageData, 8> ResultData;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 131103);

  bool Check(const void* rawData, std::size_t limit)
  {
    if (limit < sizeof(Header))
    {
      return false;
    }
    const Header& header = *static_cast<const Header*>(rawData);
    if (header.TRDosROM > 1)
    {
      return false;
    }
    const uint_t curPage = header.Port7FFD & 7;
    const bool pageDuped = curPage == 2 || curPage == 5;
    const std::size_t origSize = pageDuped ? sizeof(header) + sizeof(PageData) : sizeof(header);
    if (limit != origSize)
    {
      return false;
    }
    //in case of duped one more page
    if (pageDuped)
    {
      const PageData& cmpPage = curPage == 2 ? header.Page2 : header.Page5;
      if (cmpPage != header.ActivePage)
      {
        return false;
      }
    }
    return true;
  }

  Formats::Packed::Container::Ptr Decode(const uint8_t* rawData)
  {
    static const uint_t PAGE_NUM_TO_INDEX[] = {2, 3, 1, 4, 5, 0, 6, 7};

    std::auto_ptr<Dump> result(new Dump(sizeof(ResultData)));
    const Header& src = *safe_ptr_cast<const Header*>(rawData);
    ResultData& dst = *safe_ptr_cast<ResultData*>(&result->front());
    const uint_t curPage = src.Port7FFD & 7;
    dst[PAGE_NUM_TO_INDEX[5]] = src.Page5;
    dst[PAGE_NUM_TO_INDEX[2]] = src.Page2;
    const bool pageDuped = curPage == 2 || curPage == 5;
    if (!pageDuped)
    {
      dst[PAGE_NUM_TO_INDEX[curPage]] = src.ActivePage;
    }
    for (uint_t page = 0, idx = 0; page < 8; ++page)
    {
      if (2 == page || 5 == page || curPage == page)
      {
        continue;
      }
      dst[PAGE_NUM_TO_INDEX[page]] = src.Pages[idx];
      ++idx;
    }
    const std::size_t origSize = pageDuped ? sizeof(src) + sizeof(PageData) : sizeof(src);
    return CreatePackedContainer(result, origSize);
  }

  const std::string FORMAT(
    "+19+"
    "?" //iff. US saves 0x00/0xff instead of normal 0x00..0x03 flags
    "+3+"
    "? 40-ff" //sp
    "00-02" //im mode
    "00-07" //border
  );
}

namespace Formats
{
  namespace Packed
  {
    class Sna128Decoder : public Decoder
    {
    public:
      Sna128Decoder()
        : Format(Binary::Format::Create(Sna128::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SNA128_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const uint8_t* const data = static_cast<const uint8_t*>(rawData.Data());
        const std::size_t availSize = rawData.Size();
        if (!Format->Match(data, availSize) || !Sna128::Check(data, availSize))
        {
          return Formats::Packed::Container::Ptr();
        }
        return Sna128::Decode(data);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateSna128Decoder()
    {
      return boost::make_shared<Sna128Decoder>();
    }
  }
}
