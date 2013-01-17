/**
*
* @file      sound/render_params.h
* @brief     Rendering parameters definition. Used as POD-helper
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __RENDER_PARAMS_H_DEFINED__
#define __RENDER_PARAMS_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <time/stamp.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Input parameters for rendering
    class RenderParameters
    {
    public:
      typedef boost::shared_ptr<const RenderParameters> Ptr;

      virtual ~RenderParameters() {}

      //! Rendering sound frequency
      virtual uint_t SoundFreq() const = 0;
      //! Frame duration in us
      virtual Time::Microseconds FrameDuration() const = 0;
      //! Loop mode
      virtual bool Looped() const = 0;
      //! Sound samples count per one frame
      virtual uint_t SamplesPerFrame() const = 0;

      static Ptr Create(Parameters::Accessor::Ptr soundParameters);
    };
  }
}

#endif //__RENDER_PARAMS_H_DEFINED__
