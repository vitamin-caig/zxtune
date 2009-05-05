#ifndef __PLAYER_ATTRS_H_DEFINED__
#define __PLAYER_ATTRS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  /// Capabilities for players
  const unsigned CAP_AYM = 1;
  const unsigned CAP_TS = 2;
  const unsigned CAP_BEEPER = 4;
  const unsigned CAP_SOUNDRIVE = 8;
  const unsigned CAP_FM = 16;

  /// Attributes for players
  const String::value_type ATTR_DESCRIPTION[] = {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', '\0'};
  const String::value_type ATTR_VERSION[] = {'V', 'e', 'r', 's', 'i', 'o', 'n', '\0'};
}

#endif //__PLAYER_ATTRS_H_DEFINED__
