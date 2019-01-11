/**
*
* @file
*
* @brief  Information interface
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
  //! @brief Common module information
  class Information
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const Information> Ptr;

    virtual ~Information() = default;

    //! Total frames count
    virtual uint_t FramesCount() const = 0;
    //! Loop position frame
    virtual uint_t LoopFrame() const = 0;
    //! Channels count
    virtual uint_t ChannelsCount() const = 0;
  };
}
