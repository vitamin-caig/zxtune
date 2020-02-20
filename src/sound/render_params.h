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

//library includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>
#include <sound/loop.h>
#include <time/duration.h>

namespace Sound
{
  //! @brief Input parameters for rendering
  class RenderParameters
  {
  public:
    typedef std::shared_ptr<const RenderParameters> Ptr;

    virtual ~RenderParameters() = default;

    virtual uint_t Version() const = 0;

    //! Rendering sound frequency
    virtual uint_t SoundFreq() const = 0;
    //! Frame duration in us
    virtual Time::Microseconds FrameDuration() const = 0;
    //! Loop mode
    virtual LoopParameters Looped() const = 0;
    //! Sound samples count per one frame
    virtual uint_t SamplesPerFrame() const = 0;

    static Ptr Create(Parameters::Accessor::Ptr soundParameters);
  };
  
  Time::Microseconds GetFrameDuration(const Parameters::Accessor& params);
  void SetFrameDuration(Parameters::Modifier& params, Time::Microseconds duration);
}
