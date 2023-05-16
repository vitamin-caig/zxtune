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

// common includes
#include <types.h>
// std includes
#include <type_traits>
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
      {}

      LinesObject(const LinesObject&) = delete;
      LinesObject& operator=(const LinesObject&) = delete;

      LinesObject(LinesObject&& rh) noexcept  // = default
        : Lines(std::move(rh.Lines))
        , Loop(rh.Loop)
      {}

      LinesObject& operator=(LinesObject&& rh) noexcept  // = default
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

      typename std::conditional<std::is_fundamental<LineType>::value, LineType, const LineType&>::type
          GetLine(uint_t pos) const
      {
        static const LineType STUB = LineType();
        return Lines.size() > pos ? Lines[pos] : STUB;
      }

      const LineType* FindLine(uint_t pos) const
      {
        return Lines.size() > pos ? &Lines[pos] : nullptr;
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
      {}

      LinesObjectWithLoopLimit(LinesObjectWithLoopLimit&& rh) noexcept  // = default
        : LinesObject<LineType>(std::move(rh))
        , LoopLimit(rh.LoopLimit)
      {}

      LinesObjectWithLoopLimit& operator=(LinesObjectWithLoopLimit&& rh) noexcept  // = default
      {
        LinesObject<LineType>::operator=(std::move(rh));
        LoopLimit = rh.LoopLimit;
        return *this;
      }

      uint_t GetLoopLimit() const
      {
        return LoopLimit;
      }

      uint_t LoopLimit;
    };
  }  // namespace Chiptune
}  // namespace Formats
