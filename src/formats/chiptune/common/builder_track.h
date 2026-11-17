/**
 *
 * @file
 *
 * @brief  Track builder interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_pattern.h"

#include "types.h"

namespace Formats::Chiptune
{
  class TrackBuilder
  {
  public:
    virtual ~TrackBuilder() = default;

    virtual PatternBuilder& StartPattern(uint_t index) = 0;
    virtual void StartChannel(uint_t index) = 0;
    virtual void SetRest() = 0;
    virtual void SetNote(uint_t note) = 0;
    virtual void SetSample(uint_t index) = 0;
    virtual void SetOrnament(uint_t index) = 0;
    virtual void SetVolume(uint_t volume) = 0;
  };
}  // namespace Formats::Chiptune
