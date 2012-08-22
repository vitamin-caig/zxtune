/*
Abstract:
  Formatting functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <format.h>
//text includes
#include <tools/text/tools.h>

namespace Strings
{
  String FormatTime(uint_t timeInFrames, uint_t frameDurationMicrosec)
  {
    const uint_t US_PER_SEC = 1000000;
    const uint_t fpsRough = US_PER_SEC / frameDurationMicrosec;
    const uint_t allSeconds = static_cast<uint_t>(uint64_t(timeInFrames) * frameDurationMicrosec / US_PER_SEC);
    const uint_t frames = timeInFrames % fpsRough;
    const uint_t seconds = allSeconds % 60;
    const uint_t minutes = (allSeconds / 60) % 60;
    const uint_t hours = allSeconds / 3600;
    return FormatTime(hours, minutes, seconds, frames);
  }

  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames)
  {
    return hours
      ? Strings::Format(Text::TIME_FORMAT_HOURS, hours, minutes, seconds, frames)
      : Strings::Format(Text::TIME_FORMAT, minutes, seconds, frames);
  }
}
