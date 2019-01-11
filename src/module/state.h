/**
*
* @file
*
* @brief  State interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <memory>

namespace Module
{
  //! @brief Runtime module status
  class State
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const State> Ptr;

    virtual ~State() = default;

    //! Current frame in track (up to Information::FramesCount)
    virtual uint_t Frame() const = 0;
  };
}
