/**
*
* @file
*
* @brief  TrackInformation interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <module/information.h>

namespace Module
{
  //! @brief Track module information
  class TrackInformation : public Information
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const TrackInformation> Ptr;

    //! Channels count
    virtual uint_t ChannelsCount() const = 0;
    //! Total positions
    virtual uint_t PositionsCount() const = 0;
    //! Loop position index
    virtual uint_t LoopPosition() const = 0;
  };
}
