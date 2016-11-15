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
      {
      }
      
      LinesObject(const LinesObject&) = delete;
      LinesObject& operator = (const LinesObject&) = delete;
      
      LinesObject(LinesObject&& rh)// = default
        : Lines(std::move(rh.Lines))
        , Loop(rh.Loop)
      {
      }
      
      LinesObject& operator = (LinesObject&& rh)// = default
      {
        Lines = std::move(rh.Lines);
        Loop = rh.Loop;
        return *this;
      }
      
      uint_t GetLoop() const
      {
        return Loop;
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

      std::vector<LineType> Lines;
      uint_t Loop;
    };
    
    template<class LineType>
    class LinesObjectWithLoopLimit : public LinesObject<LineType>
    {
    public:
      LinesObjectWithLoopLimit()
        : LinesObject<LineType>()
        , LoopLimit()
      {
      }
       
      LinesObjectWithLoopLimit(LinesObjectWithLoopLimit&& rh)// = default
        : LinesObject<LineType>(std::move(rh))
        , LoopLimit(rh.LoopLimit)
      {
      }
      
      LinesObjectWithLoopLimit& operator = (LinesObjectWithLoopLimit&& rh)// = default
      {
        LinesObject<LineType>::operator = (std::move(rh));
        LoopLimit = rh.LoopLimit;
        return *this;
      }
      
      uint_t GetLoopLimit() const
      {
        return LoopLimit;
      }
      
      uint_t LoopLimit;
    };
  }
}
