/**
 *
 * @file
 *
 * @brief  Rendering parameters definition. Used as POD-helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/loop.h"
#include "parameters/accessor.h"

namespace Sound
{
  //! @brief Input parameters for rendering
  class RenderParameters
  {
  public:
    using Ptr = std::shared_ptr<const RenderParameters>;

    virtual ~RenderParameters() = default;

    virtual uint_t Version() const = 0;

    //! Rendering sound frequency
    virtual uint_t SoundFreq() const = 0;

    static Ptr Create(Parameters::Accessor::Ptr soundParameters);
  };

  Module::LoopParameters GetLoopParameters(const Parameters::Accessor& params);
  uint_t GetSoundFrequency(const Parameters::Accessor& params);
}  // namespace Sound
