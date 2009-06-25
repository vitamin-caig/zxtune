#ifndef __PLAYER_ATTRS_H_DEFINED__
#define __PLAYER_ATTRS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  /// Capabilities for players
  enum
  {
    //devices
    CAP_DEVICE_MASK 	= 0xff,
    CAP_DEV_AYM 		= 0x1,
    CAP_DEV_BEEPER 		= 0x2,
    CAP_DEV_SOUNDRIVE 	= 0x4,
    CAP_DEV_FM 		= 0x8,
    
    //storages
    CAP_STORAGE_MASK 	= 0xff00,
    CAP_STOR_CONTAINER 	=  0x100,
    CAP_STOR_MULTITRACK 	=  0x200,
    CAP_STOR_SCANER 		=  0x400,
    
    //conversions
    CAP_CONVERSION_MASK = 0xffff0000,
    CAP_CONV_RAW 	=  0x10000,
    CAP_CONV_OUT 	=  0x20000,
    CAP_CONV_PSG 	=  0x40000,
    CAP_CONV_YM 	=  0x80000,
    CAP_CONV_ZX50 	= 0x100000,
    CAP_CONV_ZXAY 	= 0x200000,
    CAP_CONV_VORTEX 	= 0x400000,
  };

  //for filtering
  const uint32_t CAP_ANY = ~uint32_t(0);

  /// Attributes for players
  const String::value_type ATTR_DESCRIPTION[] = {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', '\0'};
  const String::value_type ATTR_VERSION[] = {'V', 'e', 'r', 's', 'i', 'o', 'n', '\0'};
  const String::value_type ATTR_SUBFORMATS[] = {'S', 'u', 'b', 'f', 'o', 'r', 'm', 'a', 't', 's', '\0'};
}

#endif //__PLAYER_ATTRS_H_DEFINED__
