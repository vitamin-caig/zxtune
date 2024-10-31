/**
 *
 * @file
 *
 * @brief  Defenition of gain-related functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <sound/gain.h>
#include <sound/receiver.h>

namespace Sound
{
  class Gainer : public Converter
  {
  public:
    using Ptr = std::shared_ptr<Gainer>;

    virtual void SetGain(Gain::Type gain) = 0;
  };

  Gainer::Ptr CreateGainer();
}  // namespace Sound
