#ifndef __PLAYER_ATTRS_H_DEFINED__
#define __PLAYER_ATTRS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  /// Capabilities for players
  const uint32_t CAP_AYM = 1;
  const uint32_t CAP_TS = 2;
  const uint32_t CAP_BEEPER = 4;
  const uint32_t CAP_SOUNDRIVE = 8;
  const uint32_t CAP_FM = 16;
  const uint32_t CAP_CONTAINER = 32;
  const uint32_t CAP_MULTITRACK = 64;
  const uint32_t CAP_SCANER = 128;

  //for filtering
  const uint32_t CAP_ANY = ~uint32_t(0);

  /// Attributes for players
  const String::value_type ATTR_DESCRIPTION[] = {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', '\0'};
  const String::value_type ATTR_VERSION[] = {'V', 'e', 'r', 's', 'i', 'o', 'n', '\0'};
  const String::value_type ATTR_SUBFORMATS[] = {'S', 'u', 'b', 'f', 'o', 'r', 'm', 'a', 't', 's', '\0'};
}

#endif //__PLAYER_ATTRS_H_DEFINED__
