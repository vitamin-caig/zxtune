/**
* 
* @file
*
* @brief  Object helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <vector>

namespace Formats
{
  namespace Chiptune
  {
    template<class LineType>
    class LinesObject
    {
    public:
      typedef LineType Line;
    
      LinesObject()
        : Loop()
        , LoopLimit()
      {
      }
      
      LinesObject(const LinesObject&) = delete;
      LinesObject& operator = (const LinesObject&) = delete;
      
      LinesObject(LinesObject&& rh)// = default
        : Loop(rh.Loop)
        , LoopLimit(rh.LoopLimit)
        , Lines(std::move(rh.Lines))
      {
      }
      
      LinesObject& operator = (LinesObject&& rh)// = default
      {
        Loop = rh.Loop;
        LoopLimit = rh.LoopLimit;
        Lines = std::move(rh.Lines);
        return *this;
      }
      
      uint_t GetLoop() const
      {
        return Loop;
      }
      
      uint_t GetLoopLimit() const
      {
        return LoopLimit;
      }

      uint_t GetSize() const
      {
        return static_cast<uint_t>(Lines.size());
      }

      const LineType& GetLine(uint_t pos) const
      {
        static const LineType STUB = LineType();
        return Lines.size() > pos ? Lines[pos] : STUB;
      }

      uint_t Loop;
      uint_t LoopLimit;
      std::vector<LineType> Lines;
    };
  }
}
