/**
 *
 * @file
 *
 * @brief  Simple cycle buffer template
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <algorithm>
#include <cstring>
#include <memory>

template<class T>
class CycleBuffer
{
public:
  explicit CycleBuffer(std::size_t size)
    : Buffer(new T[size])
    , BufferStart(Buffer.get())
    , BufferEnd(BufferStart + size)
    , FreeCursor(BufferStart)
    , FreeAvailable(size)
    , BusyCursor(BufferStart)
    , BusyAvailable(0)
  {}

  void Reset()
  {
    FreeCursor = BufferStart;
    FreeAvailable = BufferEnd - BufferStart;
    BusyCursor = BufferStart;
    BusyAvailable = 0;
  }

  std::size_t Put(const T* src, std::size_t count)
  {
    const std::size_t toPut = std::min(count, FreeAvailable);
    const T* input = src;
    for (std::size_t rest = toPut; rest != 0;)
    {
      const std::size_t availFree = BusyCursor <= FreeCursor ? BufferEnd - FreeCursor : BusyCursor - FreeCursor;
      const std::size_t toCopy = std::min(availFree, rest);
      std::memcpy(FreeCursor, input, toCopy * sizeof(T));
      input += toCopy;
      rest -= toCopy;
      FreeCursor += toCopy;
      if (FreeCursor == BufferEnd)
      {
        FreeCursor = BufferStart;
      }
    }
    FreeAvailable -= toPut;
    BusyAvailable += toPut;
    return toPut;
  }

  std::size_t Peek(std::size_t count, const T*& part1Start, std::size_t& part1Size, const T*& part2Start,
                   std::size_t& part2Size) const
  {
    if (const std::size_t toPeek = std::min(count, BusyAvailable))
    {
      part1Start = BusyCursor;
      part1Size = toPeek;
      part2Start = 0;
      part2Size = 0;
      if (BusyCursor >= FreeCursor)
      {
        // possible split
        const std::size_t availAtEnd = BufferEnd - BusyCursor;
        if (toPeek > availAtEnd)
        {
          part1Size = availAtEnd;
          part2Start = BufferStart;
          part2Size = toPeek - availAtEnd;
        }
      }
      return toPeek;
    }
    return 0;
  }

  std::size_t Consume(std::size_t count)
  {
    const std::size_t toConsume = std::min(count, BusyAvailable);
    BusyCursor += toConsume;
    while (BusyCursor >= BufferEnd)
    {
      BusyCursor = BufferStart + (BusyCursor - BufferEnd);
    }
    FreeAvailable += toConsume;
    BusyAvailable -= toConsume;
    return toConsume;
  }

private:
  const std::unique_ptr<T[]> Buffer;
  T* const BufferStart;
  const T* const BufferEnd;
  T* FreeCursor;
  std::size_t FreeAvailable;
  const T* BusyCursor;
  std::size_t BusyAvailable;
};
