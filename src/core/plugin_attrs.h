/**
 *
 * @file
 *
 * @brief  Plugins attributes
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace ZXTune
{
  class PluginId : public StringView
  {
    constexpr PluginId(const char* str, std::size_t size)
      : StringView(str, size)
    {}

  public:
    friend constexpr PluginId operator""_id(const char*, std::size_t) noexcept;
  };

  constexpr PluginId operator"" _id(const char* str, std::size_t size) noexcept
  {
    return PluginId(str, size);
  }

  /*
    Type - Module - Device type - AY38910
         |        |             - Turbosound
         |        |             - Beeper
         |        |             - YM2203
         |        |             - TURBOFM
         |        |             - DAC
         |        |             - SAA1099
         |        |             - MOS6581
         |        |             - SPC700
         |        |             - Multi
         |        |
         |        - Conversions - OUT
         |                      - PSG
         |                      - YM
         |                      - ZX50
         |                      - TXT
         |                      - AYDUMP
         |                      - FYM
         |
         - Container - User type - Disk image
                     |           - Snapshot
                     |           - Archive
                     |           - Compressor
                     |           - Decompiler
                     |           - MultitrackModule
                     |           - Scaner
                     |
                     - Traits - Plain
                              - OnceApplied
                              - Multifile
                              - Directories
  */

  namespace Capabilities
  {
    namespace Category
    {
      // clang-format off
      enum
      {
        MODULE    = 0x00000000,
        CONTAINER = 0x10000000,

        MASK      = 0xf0000000
      };
      // clang-format on
    }  // namespace Category

    namespace Module
    {
      namespace Type
      {
        // clang-format off
        enum
        {
          TRACK      = 0x00000000,
          STREAM     = 0x00000001,
          MEMORYDUMP = 0x00000002,
          MULTI      = 0x00000003,

          MASK       = 0x0000000f
        };
        // clang-format on
      }  // namespace Type

      namespace Device
      {
        // clang-format off
        enum
        {
          AY38910    = 0x00000010,
          TURBOSOUND = 0x00000020,
          BEEPER     = 0x00000040,
          YM2203     = 0x00000080,
          TURBOFM    = 0x00000100,
          DAC        = 0x00000200,
          SAA1099    = 0x00000400,
          MOS6581    = 0x00000800,
          SPC700     = 0x00001000,
          MULTI      = 0x00002000,
          RP2A0X     = 0x00004000,
          LR35902    = 0x00008000,
          CO12294    = 0x00010000,
          HUC6270    = 0x00020000,

          MASK       = 0x000ffff0
        };
        // clang-format on
      }  // namespace Device

      namespace Conversion
      {
        // clang-format off
        enum
        {
          OUT        = 0x00100000,
          PSG        = 0x00200000,
          YM         = 0x00400000,
          ZX50       = 0x00800000,
          TXT        = 0x01000000,
          AYDUMP     = 0x02000000,
          FYM        = 0x04000000,

          MASK       = 0x07f00000
        };
        // clang-format on
      }  // namespace Conversion

      namespace Traits
      {
        // clang-format off
        enum
        {
          //! Plugin may produce multifile tracks
          MULTIFILE  = 0x08000000,

          MASK       = 0x08000000
        };
        // clang-format on
      }  // namespace Traits

      static_assert(0 == (Category::MASK & Type::MASK & Device::MASK & Conversion::MASK & Traits::MASK),
                    "Masks conflict");
    }  // namespace Module

    namespace Container
    {
      namespace Type
      {
        // clang-format off
        enum
        {
          ARCHIVE    = 0x00000000,
          COMPRESSOR = 0x00000001,
          SNAPSHOT   = 0x00000002,
          DISKIMAGE  = 0x00000003,
          DECOMPILER = 0x00000004,
          MULTITRACK = 0x00000005,
          SCANER     = 0x00000006,

          MASK       = 0x0000000f
        };
        // clang-format on
      }  // namespace Type

      namespace Traits
      {
        // clang-format off
        enum
        {
          //! Container may have directory structure
          DIRECTORIES = 0x00000010,
          //! Use plain transformation, can be covered by scaner
          PLAIN       = 0x00000020,

          MASK        = 0x000000f0
        };
        // clang-format on
      }  // namespace Traits

      static_assert(0 == (Category::MASK & Type::MASK & Traits::MASK), "Masks conflict");
    }  // namespace Container
  }    // namespace Capabilities
}  // namespace ZXTune
