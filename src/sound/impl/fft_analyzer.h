/**
 *
 * @file
 *
 * @brief  FFT analyzer interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/analyzer.h"

#include <cstddef>
#include <memory>

namespace Sound
{
  struct Sample;

  //! @brief %Sound analyzer interface
  class FFTAnalyzer : public Analyzer
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<FFTAnalyzer>;

    // TODO: use std::span when available
    virtual void FeedSound(const Sample* samples, std::size_t count) = 0;

    static Ptr Create();
  };
}  // namespace Sound
