/**
* 
* @file
*
* @brief  Simple ornament implementation
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <vector>

namespace Module
{
  // Commonly, ornament is just a set of tone offsets
  class SimpleOrnament
  {
  public:
    SimpleOrnament()
      : Loop()
      , Lines()
    {
    }

    template<class It>
    SimpleOrnament(uint_t loop, It from, It to)
      : Loop(loop)
      , Lines(from, to)
    {
    }

    template<class It>
    SimpleOrnament(It from, It to)
      : Loop(0)
      , Lines(from, to)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    int_t GetLine(uint_t pos) const
    {
      return Lines.size() > pos ? Lines[pos] : 0;
    }
  private:
    uint_t Loop;
    std::vector<int_t> Lines;
  };
}
