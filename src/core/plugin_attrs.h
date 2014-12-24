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
  //! @brief Set of capabilities for plugins
  enum
  {
    //! Device-related capabilities
    CAP_DEVICE_MASK    = 0xfff,
    //! Single AY/YM support
    CAP_DEV_AY38910    = 0x001,
    //! Double AY/YM support
    CAP_DEV_TURBOSOUND = 0x002,
    //! Beeper support
    CAP_DEV_BEEPER     = 0x004,
    //! Single FM chip
    CAP_DEV_YM2203     = 0x008,
    //! Double FM chip support
    CAP_DEV_TURBOFM    = 0x010,
    //! DAC-based devices support
    CAP_DEV_DAC        = 0x020,
    //! SAA1099 support
    CAP_DEV_SAA1099    = 0x040,
    //! SID (MOS6581/MOS8580) support
    CAP_DEV_MOS6581    = 0x080,
    //! MIDI support
    CAP_DEV_MIDI       = 0x100,

    //! Storages-related capabilities
    CAP_STORAGE_MASK    = 0xff000,
    //! Single module
    CAP_STOR_MODULE     = 0x01000,
    //! Supporting for container feature- raw dump transformation
    CAP_STOR_CONTAINER  = 0x02000,
    //! Supporting for multitrack feature
    CAP_STOR_MULTITRACK = 0x04000,
    //! Supporting for scanning feature
    CAP_STOR_SCANER     = 0x08000,
    //! Use plain transformation, can be covered by scaner
    CAP_STOR_PLAIN      = 0x10000,
    //! Supporting for directories in paths
    CAP_STOR_DIRS       = 0x20000,

    //! Conversion-related capabilities
    CAP_CONVERSION_MASK = 0xfff00000,
    //! Support raw conversion (save ripped data)
    CAP_CONV_RAW        = 0x00100000,
    //! Support .out format conversion
    CAP_CONV_OUT        = 0x00200000,
    //! Support .psg format conversion
    CAP_CONV_PSG        = 0x00400000,
    //! Support .ym format conversion
    CAP_CONV_YM         = 0x00800000,
    //! Support .zx50 format conversion
    CAP_CONV_ZX50       = 0x01000000,
    //! Support text vortex format conversion
    CAP_CONV_TXT        = 0x02000000,
    //! Support raw aydump conversion
    CAP_CONV_AYDUMP     = 0x04000000,
    //! Support .fym format conversion
    CAP_CONV_FYM        = 0x08000000,
  };
}
