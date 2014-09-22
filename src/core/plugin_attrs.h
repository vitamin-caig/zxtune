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
    CAP_DEVICE_MASK    = 0xff,
    //! Single AY/YM support
    CAP_DEV_AY38910    = 0x01,
    //! Double AY/YM support
    CAP_DEV_TURBOSOUND = 0x02,
    //! Beeper support
    CAP_DEV_BEEPER     = 0x04,
    //! Single FM chip
    CAP_DEV_YM2203     = 0x08,
    //! Double FM chip support
    CAP_DEV_TURBOFM    = 0x10,
    //! DAC-based devices support
    CAP_DEV_DAC        = 0x20,
    //! SAA1099 support
    CAP_DEV_SAA1099    = 0x40,
    //! SID (MOS6581/MOS8580) support
    CAP_DEV_MOS6581    = 0x80,

    //! Storages-related capabilities
    CAP_STORAGE_MASK    = 0xff00,
    //! Single module
    CAP_STOR_MODULE     = 0x0100,
    //! Supporting for container feature- raw dump transformation
    CAP_STOR_CONTAINER  = 0x0200,
    //! Supporting for multitrack feature
    CAP_STOR_MULTITRACK = 0x0400,
    //! Supporting for scanning feature
    CAP_STOR_SCANER     = 0x0800,
    //! Use plain transformation, can be covered by scaner
    CAP_STOR_PLAIN      = 0x1000,
    //! Supporting for directories in paths
    CAP_STOR_DIRS       = 0x2000,

    //! Conversion-related capabilities
    CAP_CONVERSION_MASK = 0xffff0000,
    //! Support raw conversion (save ripped data)
    CAP_CONV_RAW        = 0x00010000,
    //! Support .out format conversion
    CAP_CONV_OUT        = 0x00020000,
    //! Support .psg format conversion
    CAP_CONV_PSG        = 0x00040000,
    //! Support .ym format conversion
    CAP_CONV_YM         = 0x00080000,
    //! Support .zx50 format conversion
    CAP_CONV_ZX50       = 0x00100000,
    //! Support text vortex format conversion
    CAP_CONV_TXT        = 0x00200000,
    //! Support raw aydump conversion
    CAP_CONV_AYDUMP     = 0x00400000,
    //! Support .fym format conversion
    CAP_CONV_FYM        = 0x00800000,
  };
}
