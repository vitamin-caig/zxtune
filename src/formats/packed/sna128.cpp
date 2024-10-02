/**
 *
 * @file
 *
 * @brief  SNA128 snapshots support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <formats/packed.h>
// std includes
#include <array>
#include <numeric>

namespace Formats::Packed
{
  namespace Sna128
  {
    using PageData = std::array<uint8_t, 16384>;

    struct Header
    {
      uint8_t RegI;
      le_uint16_t RegHL_;
      le_uint16_t RegDE_;
      le_uint16_t RegBC_;
      le_uint16_t RegAF_;
      le_uint16_t RegHL;
      le_uint16_t RegDE;
      le_uint16_t RegBC;
      le_uint16_t RegIY;
      le_uint16_t RegIX;
      //+0x13 %000000xx
      uint8_t IFF;
      uint8_t RegR;
      le_uint16_t RegAF;
      le_uint16_t RegSP;
      //+0x19 0/1/2
      uint8_t ImMode;
      //+0x1a 0..7
      uint8_t Border;
      PageData Page5;
      PageData Page2;
      PageData ActivePage;
      le_uint16_t RegPC;
      uint8_t Port7FFD;
      //+0xc01e 0/1
      uint8_t TRDosROM;
      PageData Pages[5];
      // optional page starts here
    };

    // 5,2,0,1,3,4,6,7
    using ResultData = std::array<PageData, 8>;

    static_assert(sizeof(Header) * alignof(Header) == 131103, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header);

    bool Check(Binary::View data)
    {
      const auto limit = data.Size();
      if (limit < sizeof(Header))
      {
        return false;
      }
      const auto& header = *data.As<Header>();
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
      // in case of duped one more page
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

    Formats::Packed::Container::Ptr Decode(Binary::View data)
    {
      static const uint_t PAGE_NUM_TO_INDEX[] = {2, 3, 1, 4, 5, 0, 6, 7};

      Binary::DataBuilder result(sizeof(ResultData));
      const Header& src = *data.As<Header>();
      auto& dst = result.Add<ResultData>();
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
      return CreateContainer(result.CaptureResult(), origSize);
    }

    const Char DESCRIPTION[] = "SNA 128k";
    const auto FORMAT =
        "?{19}"
        "00|01|02|03|04|ff"  // iff. US saves 0x00/0x04/0xff instead of normal 0x00..0x03 flags
        "?{3}"
        "? 40-ff"  // sp
        "00-02"    // im mode
        "00-07"    // border
        ""sv;
  }  // namespace Sna128

  class Sna128Decoder : public Decoder
  {
  public:
    Sna128Decoder()
      : Format(Binary::CreateFormat(Sna128::FORMAT, Sna128::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return Sna128::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const Binary::View data(rawData);
      if (!Format->Match(data))
      {
        return {};
      }
      if (!Sna128::Check(data))
      {
        return {};
      }
      return Sna128::Decode(data);
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateSna128Decoder()
  {
    return MakePtr<Sna128Decoder>();
  }
}  // namespace Formats::Packed
