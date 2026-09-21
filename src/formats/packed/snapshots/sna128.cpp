/**
 *
 * @file
 *
 * @brief  SNA128 snapshots support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/common/container.h"

#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "formats/packed/decoder.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "pointers.h"

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

    Container::Ptr TryDecode(Binary::View data)
    {
      static const uint_t PAGE_NUM_TO_INDEX[] = {2, 3, 1, 4, 5, 0, 6, 7};
      const auto limit = data.Size();
      if (limit < sizeof(Header))
      {
        return {};
      }
      const auto& header = *data.As<Header>();
      if (header.TRDosROM > 1)
      {
        return {};
      }
      const uint_t curPage = header.Port7FFD & 7;
      const bool pageDuped = curPage == 2 || curPage == 5;
      const std::size_t origSize = pageDuped ? sizeof(header) + sizeof(PageData) : sizeof(header);
      if (limit != origSize)
      {
        return {};
      }
      // in case of duped one more page
      if (pageDuped)
      {
        const PageData& cmpPage = curPage == 2 ? header.Page2 : header.Page5;
        if (cmpPage != header.ActivePage)
        {
          return {};
        }
      }

      Binary::DataBuilder result(sizeof(ResultData));
      auto& dst = result.Add<ResultData>();
      dst[PAGE_NUM_TO_INDEX[5]] = header.Page5;
      dst[PAGE_NUM_TO_INDEX[2]] = header.Page2;
      if (!pageDuped)
      {
        dst[PAGE_NUM_TO_INDEX[curPage]] = header.ActivePage;
      }
      for (uint_t page = 0, idx = 0; page < 8; ++page)
      {
        if (2 == page || 5 == page || curPage == page)
        {
          continue;
        }
        dst[PAGE_NUM_TO_INDEX[page]] = header.Pages[idx];
        ++idx;
      }
      return CreateContainer(result.CaptureResult(), origSize);
    }

    const auto DESCRIPTION = "SNA 128k"sv;
    const auto FORMAT =
        "?{19}"
        "00|01|02|03|04|ff"  // iff. US saves 0x00/0x04/0xff instead of normal 0x00..0x03 flags
        "?{3}"
        "? 40-ff"  // sp
        "00-02"    // im mode
        "00-07"    // border
        ""sv;

    class Decoder : public Packed::Decoder
    {
    public:
      StringView GetDescription() const override
      {
        return DESCRIPTION;
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
        return TryDecode(data);
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(FORMAT, MIN_SIZE);
    };
  }  // namespace Sna128

  Decoder::Ptr CreateSna128Decoder()
  {
    return MakePtr<Sna128::Decoder>();
  }
}  // namespace Formats::Packed
