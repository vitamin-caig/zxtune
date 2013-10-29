/**
* 
* @file
*
* @brief  Module renderer (former Player) interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_types.h>

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
    //! @return true if next frame can be rendered
    virtual bool RenderFrame() = 0;

    //! @brief Performing reset to initial state
    virtual void Reset() = 0;

    //! @brief Seeking
    //! @param frame Number of specified frame
    //! @note Seeking out of range is safe, but state will be MODULE_PLAYING untill next RenderFrame call happends.
    //! @note It produces only the flush
    virtual void SetPosition(uint_t frame) = 0;
  };
}
