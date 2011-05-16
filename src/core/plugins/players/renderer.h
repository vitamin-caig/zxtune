/*
Abstract:
  Module renderer interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_RENDERER_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_RENDERER_H_DEFINED__

//library includes
#include <core/module_types.h> //for Module::Analyze::ChannelsState
#include <sound/receiver.h>    //for Sound::MultichannelReceiver
//boost includes
#include <boost/weak_ptr.hpp>

//forward declarations
class Error;

namespace ZXTune
{
  namespace Sound
  {
    class RenderParameters;
  }

  namespace Module
  {
    //! @brief %Module player interface
    class Renderer
    {
    public:
      //! @brief Generic pointer type
      typedef boost::shared_ptr<Renderer> Ptr;

      virtual ~Renderer() {}

      //! @brief Current tracking status
      virtual TrackState::Ptr GetTrackState() const = 0;

      //! @brief Getting analyzer interface
      virtual Analyzer::Ptr GetAnalyzer() const = 0;

      //! @brief Rendering single frame and modifying internal state
      //! @param params %Sound rendering-related parameters
      //! @return true if next frame can be rendered
      virtual bool RenderFrame(const Sound::RenderParameters& params) = 0;

      //! @brief Performing reset to initial state
      virtual void Reset() = 0;

      //! @brief Seeking
      //! @param frame Number of specified frame
      //! @note Seeking out of range is safe, but state will be MODULE_PLAYING untill next RenderFrame call happends.
      //! @note It produces only the flush
      virtual void SetPosition(uint_t frame) = 0;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_RENDERER_H_DEFINED__
