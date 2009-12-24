/*
Abstract:
  Plugins attributes

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PLUGIN_ATTRS_H_DEFINED__
#define __CORE_PLUGIN_ATTRS_H_DEFINED__

namespace ZXTune
{
  //! Capabilities for plugins
  enum
  {
    //! Device-related capabilities
    CAP_DEVICE_MASK    = 0xff,
    //! Supporting for AY/YM (count of supported devices)
    CAP_DEV_AYM_MASK   = 0x03,
    //! Single AYM support
    CAP_DEV_AYM        = 0x01,
    //! Double AYM support
    CAP_DEV_TS         = 0x02,
    //! Supporting for beeper
    CAP_DEV_BEEPER     = 0x04,
    //! Supporting for FM
    CAP_DEV_FM         = 0x08,
    //! Supporting for DAC (count of supported channels)
    CAP_DEV_DAC_MASK   = 0x70,
    CAP_DEV_1DAC       = 0x10,
    CAP_DEV_2DAC       = 0x20,
    CAP_DEV_3DAC       = 0x30,
    CAP_DEV_4DAC       = 0x40,

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
    
    //! Conversion-related capabilities
    CAP_CONVERSION_MASK = 0xffff0000,
    CAP_CONV_RAW        = 0x00010000,
    CAP_CONV_OUT        = 0x00020000,
    CAP_CONV_PSG        = 0x00040000,
    CAP_CONV_YM         = 0x00080000,
    CAP_CONV_ZX50       = 0x00100000,
    CAP_CONV_ZXAY       = 0x00200000,
    CAP_CONV_VORTEX     = 0x00400000,
  };
}

#endif //__CORE_PLUGINS_ATTRS_H_DEFINED__
