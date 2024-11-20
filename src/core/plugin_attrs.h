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

#include "string_view.h"
#include "types.h"

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
    return {str, size};
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
      constexpr uint_t MODULE = 0x00000000;
      constexpr uint_t CONTAINER = 0x10000000;

      constexpr uint_t MASK = 0xf0000000;
    }  // namespace Category

    namespace Module
    {
      namespace Type
      {
        constexpr uint_t TRACK = 0x00000000;
        constexpr uint_t STREAM = 0x00000001;
        constexpr uint_t MEMORYDUMP = 0x00000002;
        constexpr uint_t MULTI = 0x00000003;

        constexpr uint_t MASK = 0x0000000f;
      }  // namespace Type

      namespace Device
      {
        constexpr uint_t AY38910 = 0x00000010;
        constexpr uint_t TURBOSOUND = 0x00000020;
        constexpr uint_t BEEPER = 0x00000040;
        constexpr uint_t YM2203 = 0x00000080;
        constexpr uint_t TURBOFM = 0x00000100;
        constexpr uint_t DAC = 0x00000200;
        constexpr uint_t SAA1099 = 0x00000400;
        constexpr uint_t MOS6581 = 0x00000800;
        constexpr uint_t SPC700 = 0x00001000;
        constexpr uint_t MULTI = 0x00002000;
        constexpr uint_t RP2A0X = 0x00004000;
        constexpr uint_t LR35902 = 0x00008000;
        constexpr uint_t CO12294 = 0x00010000;
        constexpr uint_t HUC6270 = 0x00020000;

        constexpr uint_t MASK = 0x000ffff0;
      }  // namespace Device

      namespace Conversion
      {
        constexpr uint_t OUT = 0x00100000;
        constexpr uint_t PSG = 0x00200000;
        constexpr uint_t YM = 0x00400000;
        constexpr uint_t ZX50 = 0x00800000;
        constexpr uint_t TXT = 0x01000000;
        constexpr uint_t AYDUMP = 0x02000000;
        constexpr uint_t FYM = 0x04000000;

        constexpr uint_t MASK = 0x07f00000;
      }  // namespace Conversion

      namespace Traits
      {
        //! Plugin may produce multifile tracks
        constexpr uint_t MULTIFILE = 0x08000000;

        constexpr uint_t MASK = 0x08000000;
      }  // namespace Traits

      static_assert(0 == (Category::MASK & Type::MASK & Device::MASK & Conversion::MASK & Traits::MASK),
                    "Masks conflict");
    }  // namespace Module

    namespace Container
    {
      namespace Type
      {
        constexpr uint_t ARCHIVE = 0x00000000;
        constexpr uint_t COMPRESSOR = 0x00000001;
        constexpr uint_t SNAPSHOT = 0x00000002;
        constexpr uint_t DISKIMAGE = 0x00000003;
        constexpr uint_t DECOMPILER = 0x00000004;
        constexpr uint_t MULTITRACK = 0x00000005;
        constexpr uint_t SCANER = 0x00000006;

        constexpr uint_t MASK = 0x0000000f;
      }  // namespace Type

      namespace Traits
      {
        //! Container may have directory structure
        constexpr uint_t DIRECTORIES = 0x00000010;
        //! Use plain transformation, can be covered by scaner
        constexpr uint_t PLAIN = 0x00000020;

        constexpr uint_t MASK = 0x000000f0;
      }  // namespace Traits

      static_assert(0 == (Category::MASK & Type::MASK & Traits::MASK), "Masks conflict");
    }  // namespace Container
  }    // namespace Capabilities
}  // namespace ZXTune
