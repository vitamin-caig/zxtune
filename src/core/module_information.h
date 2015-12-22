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
//boost includes
#include <boost/shared_ptr.hpp>

namespace Module
{
  //! @brief Common module information
  class Information
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Information> Ptr;

    virtual ~Information() {}

    //! Total positions
    virtual uint_t PositionsCount() const = 0;
    //! Loop position index
    virtual uint_t LoopPosition() const = 0;
    //! Total patterns count
    virtual uint_t PatternsCount() const = 0;
    //! Total frames count
    virtual uint_t FramesCount() const = 0;
    //! Loop position frame
    virtual uint_t LoopFrame() const = 0;
    //! Channels count
    virtual uint_t ChannelsCount() const = 0;
    //! Initial tempo
    virtual uint_t Tempo() const = 0;
  };
}
