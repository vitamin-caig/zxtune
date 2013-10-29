/**
* 
* @file
*
* @brief  Fast analogue of std::set<uint_t>
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <contract.h>
#include <types.h>
//std includes
#include <algorithm>
#include <vector>

class Indices
{
public:
  Indices(uint_t min, uint_t max)
    : Min(min)
    , Max(max)
    , Values(1 + (max - min) / BITS_PER_MASK)
    , MinValue(Max)
    , MaxValue(Min)
    , Size(0)
  {
  }

  template<class It>
  void Assign(It from, It to)
  {
    Clear();
    Insert(from, to);
  }

  void Clear()
  {
    MinValue = Max;
    MaxValue = Min;
    Size = 0;
    std::fill(Values.begin(), Values.end(), 0);
  }

  void Insert(uint_t val)
  {
    Require(val >= Min && val <= Max);
    const Position pos = GetPosition(val);
    uint_t& msk = Values[pos.Offset];
    if (0 == (msk & pos.Mask))
    {
      msk |= pos.Mask;
      ++Size;
      MinValue = std::min(MinValue, val);
      MaxValue = std::max(MaxValue, val);
    }
  }

  template<class It>
  void Insert(It from, It to)
  {
    std::for_each(from, to, std::bind1st(std::mem_fun<void, Indices, uint_t>(&Indices::Insert), this));
  }

  bool Contain(uint_t val) const
  {
    if (val < Min || val > Max)
    {
      return false;
    }
    const Position pos = GetPosition(val);
    return 0 != (Values[pos.Offset] & pos.Mask);
  }

  bool Empty() const
  {
    return MinValue > MaxValue;
  }

  uint_t Count() const
  {
    return Size;
  }

  uint_t Minimum() const
  {
    Require(!Empty());
    return MinValue;
  }

  uint_t Maximum() const
  {
    Require(!Empty());
    return MaxValue;
  }

  class Iterator
  {
  public:
    explicit Iterator(const Indices& cont)
      : Container(cont)
      , Idx(0)
      , Mask(1)
      , Ptr(&cont.Values.front())
    {
      SkipSparsed();
    }

    typedef bool (Iterator::*BoolType)() const;

    operator BoolType () const
    {
      return IsValid() ? &Iterator::IsValid : 0;
    }

    uint_t operator * () const
    {
      return Idx + Container.Min;
    }

    void operator ++ ()
    {
      Next();
      SkipSparsed();
    }
  private:
    bool IsValid() const
    {
      return Idx <= Container.MaxValue - Container.Min;
    }

    bool HasValue() const
    {
      return 0 != (*Ptr & Mask);
    }

    void Next()
    {
      ++Idx;
      if (0 == (Mask <<= 1))
      {
        ++Ptr;
        Mask = 1;
      }
    }

    void SkipSparsed()
    {
      while (IsValid() && !HasValue())
      {
        Next();
      }
    }
  private:
    const Indices& Container;
    uint_t Idx;
    uint_t Mask;
    const uint_t* Ptr;
  };

  Iterator Items() const
  {
    return Iterator(*this);
  }
private:
  struct Position
  {
    uint_t Offset;
    uint_t Mask;

    Position(uint_t off, uint_t msk)
      : Offset(off)
      , Mask(msk)
    {
    }
  };

  Position GetPosition(uint_t val) const
  {
    const uint_t off = val - Min;
    return Position(off / BITS_PER_MASK, uint_t(1) << (off % BITS_PER_MASK));
  }
private:
  static const uint_t BITS_PER_MASK = 8 * sizeof(uint_t);
  const uint_t Min;
  const uint_t Max;
  std::vector<uint_t> Values;
  uint_t MinValue;
  uint_t MaxValue;
  uint_t Size;
};
