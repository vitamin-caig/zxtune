/**
 *
 * @file
 *
 * @brief  Z80 snapshots support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/packed.h>
// std includes
#include <array>
#include <numeric>
#include <utility>

namespace Formats::Packed
{
  namespace Z80
  {
    struct Version1_45
    {
      struct Header
      {
        le_uint16_t RegFA;
        le_uint16_t RegBC;
        le_uint16_t RegHL;
        le_uint16_t RegPC;
        le_uint16_t RegSP;
        le_uint16_t RegRI;
        // 0 - R7
        // 1..3 - border
        // 4 - basic SamRam
        // 5 - compressed
        // 6..7 - not used
        // if Flag1 == 255 then Flag1 = 1;
        uint8_t Flag1;
        le_uint16_t RegDE;
        le_uint16_t RegBC_;
        le_uint16_t RegDE_;
        le_uint16_t RegHL_;
        le_uint16_t RegFA_;
        le_uint16_t RegIY;
        le_uint16_t RegIX;
        uint8_t Iff1;
        uint8_t Iff2;
        // 0..1 - im mode
        // 2 - spectrum2 emulation
        // 3 - double int freq
        // 4..5 - videosync type
        // 6..7 - joystick type
        uint8_t Flag2;
        // 48kb of data

        enum
        {
          COMPRESSED = 32
        };
      };

      static const StringView DESCRIPTION;
      static const StringView HEADER;
      static const StringView FOOTER;
      static const std::size_t MIN_SIZE;
      static const std::size_t MAX_SIZE;

      static Formats::Packed::Container::Ptr Decode(Binary::InputStream& stream);
    };

    struct Version2_0
    {
      struct Header : Version1_45::Header
      {
        le_uint16_t AdditionalSize;
        le_uint16_t PC;
        uint8_t HardwareMode;
        uint8_t Port7ffd;
        uint8_t Interface2ROM;  // 00/ff
        // 0 - R emulated
        // 1 - LDIR emulated
        uint8_t Flag3;
        uint8_t Portfffd;
        uint8_t AYRegisters[16];

        // 16kb pages of data

        static const StringView FORMAT;
      };

      struct MemoryPage
      {
        static const std::size_t UNCOMPRESSED = 0xffff;

        le_uint16_t DataSize;
        uint8_t Number;
      };

      enum HardwareTypes
      {
        Ver_48k = 0,
        Ver_48k_iface1,
        Ver_SamRam,
        Ver128k,
        Ver128k_iface1,
        // some of the emulators write such hardware in ver2 snapshots
        Ver_Pentagon = 9,
        Ver_Scorpion  // 16 pages
      };

      static const StringView DESCRIPTION;
      static const StringView FORMAT;
      static const std::size_t MIN_SIZE;
      static const std::size_t ADDITIONAL_SIZE = 23;

      static Formats::Packed::Container::Ptr Decode(Binary::InputStream& stream);
    };

    struct Version3_0
    {
      struct Header : Version2_0::Header
      {
        le_uint16_t TicksLo;
        uint8_t TicksHi;
        uint8_t Flag4;      // 00
        uint8_t GordonROM;  // 00/ff
        uint8_t M128ROM;    // 00
        uint8_t Rom0000;    // 00/ff
        uint8_t Rom2000;    // 00/ff
        uint8_t CustomJoystick[10];
        uint8_t CustomJoystickKbd[10];
        uint8_t GordonType;          // 00/01/10
        uint8_t InhibitKeyStatus;    // 00/ff
        uint8_t InhibitIfaceStatus;  // 00/ff
      };

      enum HardwareTypes
      {
        Ver_48k = 0,
        Ver_48k_iface1,
        // according to http://www.worldofspectrum.org/faq/reference/z80format.htm, SamRam == 2
        Ver_SamRam,
        Ver_48k_mgt,
        Ver_128k,
        Ver_128k_iface1,
        Ver_128k_mgt,
        // added by XZX
        Ver_Plus3,
        Ver_Plus3_alt,
        Ver_Pentagon,
        Ver_Scorpion  // 16 pages
      };

      static const StringView DESCRIPTION;
      static const StringView FORMAT;
      static const std::size_t MIN_SIZE;

      static Formats::Packed::Container::Ptr Decode(Binary::InputStream& stream);
    };

    const StringView Version1_45::DESCRIPTION = "Z80 v1.45"sv;
    const StringView Version1_45::HEADER =
        "(\?\?){6}"                      // skip registers
        "%001xxxxx"                      // take into account only compressed data
        "(\?\?){7}"                      // skip registers
        "00|01|ff"                       // iff1
        "00|01|ff"                       // iff2
        "%xxxxxx00|%xxxxxx01|%xxxxxx10"  // im3 cannot be
        ""sv;
    const StringView Version1_45::FOOTER = "00eded00"sv;

    // even if all 48kb are compressed, minimal compressed size is 4 bytes for each 255 sequenced bytes + final marker
    const std::size_t Version1_45::MIN_SIZE = sizeof(Version1_45::Header) + 4 * (49152 / 255) + 4;
    const std::size_t Version1_45::MAX_SIZE = sizeof(Version1_45::Header) + 49152 + 4;

    const StringView Version2_0::DESCRIPTION = "Z80 v2.x"sv;
    const StringView Version2_0::FORMAT =
        "(\?\?){3}"  // skip registers
        "0000"       // PC is 0
        "(\?\?){2}"
        "%00xxxxxx"
        "(\?\?){7}"                      // skip registers
        "00|01|ff"                       // iff1
        "00|01|ff"                       // iff2
        "%xxxxxx00|%xxxxxx01|%xxxxxx10"  // im3 cannot be
        "1700"                           // additional size is 23
        "\?\?"                           // PC
        "00-04|09"                       // Mode
        "\?"                             // 7ffd
        "00|ff"                          // Interface2ROM
        "%000000xx"                      // Flag3
        "\?"                             // fffd
        "\?{16}"                         // AYPorts
        ""sv;

    // at least 3 pages by 16384 bytes each
    const std::size_t Version2_0::MIN_SIZE =
        sizeof(Version2_0::Header) + 3 * (sizeof(Version2_0::MemoryPage) + 4 * (16384 / 255));

    const StringView Version3_0::DESCRIPTION = "Z80 v3.x"sv;
    const StringView Version3_0::FORMAT =
        "(\?\?){3}"  // skip registers
        "0000"       // PC is 0
        "(\?\?){2}"
        "%00xxxxxx"
        "(\?\?){7}"                      // skip registers
        "00|01|ff"                       // iff1
        "00|01|ff"                       // iff2
        "%xxxxxx00|%xxxxxx01|%xxxxxx10"  // im3 cannot be
        "36|3700"                        // additional size is 54 or 55 for XZX files
        "\?\?"                           // PC
        "00-0a"                          // Mode
        "\?"                             // 7ffd
        "00|ff"                          // Interface2ROM
        "%000000xx"                      // Flag3
        "\?"                             // fffd
        "\?{16}"                         // AYPorts
        "\?\?\?"                         // Ticks
        "00"                             // Flag4
        "00|ff"                          // GordonROM
        "00"                             // M128ROM
        "00|ff{2}"                       // ROM/Cache
        "\?{20}"                         // joystick
        "00|01|10"                       // GordonType
        "00|ff{2}"                       // Inhibitstate
        ""sv;

    // at least 3 pages by 16384 bytes each
    const std::size_t Version3_0::MIN_SIZE =
        sizeof(Version3_0::Header) + 3 * (sizeof(Version2_0::MemoryPage) + 4 * (16384 / 255));

    static_assert(sizeof(Version1_45::Header) * alignof(Version1_45::Header) == 30, "Invalid layout");
    static_assert(sizeof(Version2_0::Header) * alignof(Version2_0::Header) == 55, "Invalid layout");
    static_assert(sizeof(Version2_0::MemoryPage) * alignof(Version2_0::MemoryPage) == 3, "Invalid layout");
    static_assert(sizeof(Version3_0::Header) * alignof(Version3_0::Header) == 86, "Invalid layout");

    std::size_t DecodeBlock(const uint8_t* src, std::size_t srcSize, uint8_t* dst, std::size_t dstSize)
    {
      const uint8_t PREFIX = 0xed;

      std::memset(dst, 0, dstSize);
      std::size_t restIn = srcSize;
      while (restIn > 0 && dstSize > 0)
      {
        if (restIn >= 4 && src[0] == PREFIX && src[1] == PREFIX)
        {
          const std::size_t count = src[2];
          const uint8_t data = src[3];
          Require(count <= dstSize);
          std::memset(dst, data, count);
          src += 4;
          restIn -= 4;
          dst += count;
          dstSize -= count;
        }
        else
        {
          *dst++ = *src++;
          --restIn;
          --dstSize;
        }
      }
      return srcSize - restIn;
    }

    void DecodeBlock(Binary::InputStream& stream, std::size_t srcSize, void* dst, std::size_t dstSize)
    {
      const auto* const src = stream.PeekRawData(srcSize);
      const auto used = DecodeBlock(src, srcSize, static_cast<uint8_t*>(dst), dstSize);
      stream.Skip(used);
    }

    Formats::Packed::Container::Ptr Version1_45::Decode(Binary::InputStream& stream)
    {
      const auto& hdr = stream.Read<Version1_45::Header>();
      const std::size_t restSize = stream.GetRestSize();
      const std::size_t TARGET_SIZE = 49152;
      const uint32_t FOOTER = 0x00eded00;
      if (0 == (hdr.Flag1 & hdr.COMPRESSED))
      {
        Require(restSize >= TARGET_SIZE);
        const auto rest = stream.ReadRestContainer();
        return CreateContainer(rest->GetSubcontainer(0, TARGET_SIZE), sizeof(hdr) + TARGET_SIZE);
      }
      Require(restSize > sizeof(FOOTER));
      Binary::DataBuilder res;
      DecodeBlock(stream, restSize - sizeof(FOOTER), res.Allocate(TARGET_SIZE), TARGET_SIZE);
      const uint32_t footer = stream.Read<le_uint32_t>();
      Require(footer == FOOTER);
      return CreateContainer(res.CaptureResult(), stream.GetPosition());
    }

    struct PlatformTraits
    {
    public:
      static const int_t NO_PAGE = -1;

      PlatformTraits(std::size_t additionalSize, uint_t hwMode, uint_t port7ffd)
      {
        if (additionalSize == Version2_0::ADDITIONAL_SIZE)
        {
          FillTraits(static_cast<Version2_0::HardwareTypes>(hwMode));
        }
        else
        {
          FillTraits(static_cast<Version3_0::HardwareTypes>(hwMode));
        }
        MinPages = Is48kLocked(port7ffd) ? 3 : Pages;
      }

      uint_t MinimalPagesCount() const
      {
        return MinPages;
      }

      uint_t PagesCount() const
      {
        return Pages;
      }

      int_t PageNumber(uint_t idx) const
      {
        return idx < Numbers.size() ? Numbers[idx] : -1;
      }

    private:
      static bool Is48kLocked(uint_t port)
      {
        return 0 == (port & 32);
      }

      void Fill48kTraits()
      {
        static const int_t VER48_PAGES[] = {
            NO_PAGE,  // ROM not need
            NO_PAGE,  // interface rom is not available
            NO_PAGE, NO_PAGE,
            1,  // p4 to 4000
            2,  // p5 to 8000
            NO_PAGE, NO_PAGE,
            0  // p8 to 0000
        };

        Pages = 3;
        Numbers.assign(VER48_PAGES, std::end(VER48_PAGES));
      }

      void FillSamRamTraits()
      {
        static const int_t SAMRAM_PAGES[] = {
            NO_PAGE,  // ROM not need
            NO_PAGE,  // interface ROM not need
            NO_PAGE,  // SamRam basic ROM not need
            NO_PAGE,  // SamRam monitor ROM not need
            1,        // p4 to 4000
            2,        // p5 to 8000
            3,        // p6 to c000 (shadow)
            4,        // p7 to 10000 (shadow)
            0         // p8 to 0000
        };

        Pages = 5;
        Numbers.assign(SAMRAM_PAGES, std::end(SAMRAM_PAGES));
      }

      void Fill128kTraits()
      {
        static const int_t VER128_PAGES[] = {
            NO_PAGE,  // old ROM not need
            NO_PAGE,  // interface ROM not need
            NO_PAGE,  // new ROM not need
            2,        // p3(0)  to  8000
            3,        // p4(1)  to  c000
            1,        // p5(2)  to  4000
            4,        // p6(3)  to 10000
            5,        // p7(4)  to 14000
            0,        // p8(5)  to  0000
            6,        // p9(6)  to 18000
            7,        // p10(7) to 1c000
        };
        Pages = 8;
        Numbers.assign(VER128_PAGES, std::end(VER128_PAGES));
      }

      void Fill256kTraits()
      {
        static const int_t VER256_PAGES[] = {
            NO_PAGE,  // old ROM not need
            NO_PAGE,  // interface ROM not need
            NO_PAGE,  // new ROM not need
            2,        // p3(0)  to  8000
            3,        // p4(1)  to  c000
            1,        // p5(2)  to  4000
            4,        // p6(3)  to 10000
            5,        // p7(4)  to 14000
            0,        // p8(5)  to  0000
            6,        // p9(6)  to 18000
            7,        // p10(7) to 1c000
            8,       9, 10, 11, 12, 13, 14, 15,
        };
        Pages = 16;
        Numbers.assign(VER256_PAGES, std::end(VER256_PAGES));
      }

      void FillTraits(Version2_0::HardwareTypes hwMode)
      {
        switch (hwMode)
        {
        case Version2_0::Ver_48k:
        case Version2_0::Ver_48k_iface1:
          Fill48kTraits();
          break;
        case Version2_0::Ver_SamRam:
          FillSamRamTraits();
          break;
        case Version2_0::Ver128k:
        case Version2_0::Ver128k_iface1:
        case Version2_0::Ver_Pentagon:
          Fill128kTraits();
          break;
        default:
          Require(false);
          break;
        }
      }

      void FillTraits(Version3_0::HardwareTypes hwMode)
      {
        switch (hwMode)
        {
        case Version3_0::Ver_48k:
        case Version3_0::Ver_48k_iface1:
        case Version3_0::Ver_48k_mgt:
          Fill48kTraits();
          break;
        case Version3_0::Ver_SamRam:
          FillSamRamTraits();
          break;
        case Version3_0::Ver_128k:
        case Version3_0::Ver_128k_iface1:
        case Version3_0::Ver_128k_mgt:
        case Version3_0::Ver_Pentagon:
          Fill128kTraits();
          break;
        case Version3_0::Ver_Scorpion:
          Fill256kTraits();
          break;
        default:
          Require(false);
          break;
        }
      }

    private:
      uint_t MinPages = 0;
      uint_t Pages = 0;
      std::vector<int_t> Numbers;
    };

    Formats::Packed::Container::Ptr DecodeNew(Binary::InputStream& stream)
    {
      const std::size_t ZX_PAGE_SIZE = 16384;
      const auto& hdr = stream.Read<Version2_0::Header>();
      const std::size_t additionalSize = hdr.AdditionalSize;
      const std::size_t readAdditionalSize = sizeof(hdr) - sizeof(Version1_45::Header) - sizeof(hdr.AdditionalSize);
      Require(additionalSize >= readAdditionalSize);
      stream.Skip(additionalSize - readAdditionalSize);
      const PlatformTraits traits(additionalSize, hdr.HardwareMode, hdr.Port7ffd);
      Binary::DataBuilder res;
      res.Allocate(ZX_PAGE_SIZE * traits.PagesCount());
      for (uint_t idx = 0; idx != traits.PagesCount(); ++idx)
      {
        const bool isPageRequired = idx < traits.MinimalPagesCount();
        if (!isPageRequired && stream.GetRestSize() < sizeof(Version2_0::MemoryPage))
        {
          break;
        }
        const auto& page = stream.Read<Version2_0::MemoryPage>();
        const int_t pageNumber = traits.PageNumber(page.Number);
        const bool isPageValid = pageNumber != PlatformTraits::NO_PAGE;
        if (!isPageRequired && !isPageValid)
        {
          break;
        }
        Require(isPageValid);
        const std::size_t pageSize = page.DataSize;
        auto* pageTarget = res.Get(pageNumber * ZX_PAGE_SIZE);
        if (pageSize == page.UNCOMPRESSED)
        {
          std::memcpy(pageTarget, stream.ReadData(ZX_PAGE_SIZE).Start(), ZX_PAGE_SIZE);
        }
        else
        {
          Require(pageSize <= stream.GetRestSize());
          DecodeBlock(stream, pageSize, pageTarget, ZX_PAGE_SIZE);
        }
      }
      return CreateContainer(res.CaptureResult(), stream.GetPosition());
    }

    Formats::Packed::Container::Ptr Version2_0::Decode(Binary::InputStream& stream)
    {
      return DecodeNew(stream);
    }

    Formats::Packed::Container::Ptr Version3_0::Decode(Binary::InputStream& stream)
    {
      return DecodeNew(stream);
    }
  }  // namespace Z80

  template<class Version>
  class Z80Decoder : public Decoder
  {
  public:
    Z80Decoder()
      : Format(Binary::CreateFormat(Version::FORMAT, Version::MIN_SIZE))
    {}

    explicit Z80Decoder(Binary::Format::Ptr format)
      : Format(std::move(format))
    {}

    String GetDescription() const override
    {
      return String{Version::DESCRIPTION};
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return {};
      }
      try
      {
        Binary::InputStream stream(rawData);
        return Version::Decode(stream);
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateZ80V145Decoder()
  {
    const Binary::Format::Ptr header = Binary::CreateFormat(Z80::Version1_45::HEADER, Z80::Version1_45::MIN_SIZE);
    const Binary::Format::Ptr footer = Binary::CreateFormat(Z80::Version1_45::FOOTER);
    const Binary::Format::Ptr format =
        Binary::CreateCompositeFormat(header, footer, Z80::Version1_45::MIN_SIZE - 4, Z80::Version1_45::MAX_SIZE - 4);
    return MakePtr<Z80Decoder<Z80::Version1_45> >(format);
  }

  Decoder::Ptr CreateZ80V20Decoder()
  {
    return MakePtr<Z80Decoder<Z80::Version2_0> >();
  }

  Decoder::Ptr CreateZ80V30Decoder()
  {
    return MakePtr<Z80Decoder<Z80::Version3_0> >();
  }
}  // namespace Formats::Packed
