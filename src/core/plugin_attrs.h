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

#include <types.h>

namespace ZXTune
{
  /// Capabilities for plugins
  enum
  {
    //devices
    CAP_DEVICE_MASK    = 0xff,
    CAP_DEV_AYM        = 0x01,
    CAP_DEV_BEEPER     = 0x02,
    CAP_DEV_SOUNDRIVE  = 0x04,
    CAP_DEV_FM         = 0x08,
    
    //storages
    CAP_STORAGE_MASK    = 0xff00,
    CAP_STOR_CONTAINER  = 0x0100,
    CAP_STOR_MULTITRACK = 0x0200,
    CAP_STOR_SCANER     = 0x0400,
    
    //conversions
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
