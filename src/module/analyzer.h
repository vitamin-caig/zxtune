/**
*
* @file
*
* @brief  Analyzer interface
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
  //! @brief %Sound analyzer interface
  class Analyzer
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const Analyzer> Ptr;

    virtual ~Analyzer() = default;

    struct ChannelState
    {
      ChannelState()
        : Band()
        , Level()
      {
      }

      uint_t Band;
      uint_t Level;
    };

    virtual void GetState(std::vector<ChannelState>& channels) const = 0;
  };
}
