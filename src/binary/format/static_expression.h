/**
 *
 * @file
 *
 * @brief  Static predicates helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "expression.h"
// common includes
#include <contract.h>
// std includes
#include <array>
#include <vector>

namespace Binary::FormatDSL
{
  class StaticPredicate
  {
  public:
    explicit StaticPredicate(const FormatDSL::Predicate& pred)
      : Mask()
      , Count()
      , Last()
    {
      for (uint_t idx = 0; idx != 256; ++idx)
      {
        if (pred.Match(idx))
        {
          Set(idx);
        }
      }
    }

    explicit StaticPredicate(uint_t val)
      : Mask()
      , Count()
      , Last()
    {
      Set(val);
    }

    bool Match(uint_t val) const
    {
      return Get(val);
    }

    bool IsAny() const
    {
      return Count == 256;
    }

    const uint_t* GetSingle() const
    {
      return Count == 1 ? &Last : nullptr;
    }

    static bool AreIntersected(const StaticPredicate& lh, const StaticPredicate& rh)
    {
      if (lh.IsAny() || rh.IsAny())
      {
        return true;
      }
      else
      {
        for (uint_t idx = 0; idx != ElementsCount; ++idx)
        {
          if (0 != (lh.Mask[idx] & rh.Mask[idx]))
          {
            return true;
          }
        }
        return false;
      }
    }

  private:
    void Set(uint_t idx)
    {
      Require(!Get(idx));
      const uint_t bit = idx % BitsPerElement;
      const std::size_t offset = idx / BitsPerElement;
      const ElementType mask = ElementType(1) << bit;
      Mask[offset] |= mask;
      ++Count;
      Last = idx;
    }

    bool Get(uint_t idx) const
    {
      const uint_t bit = idx % BitsPerElement;
      const std::size_t offset = idx / BitsPerElement;
      const ElementType mask = ElementType(1) << bit;
      return 0 != (Mask[offset] & mask);
    }

  private:
    using ElementType = uint_t;
    static const std::size_t BitsPerElement = 8 * sizeof(ElementType);
    static const std::size_t ElementsCount = 256 / BitsPerElement;
    std::array<ElementType, ElementsCount> Mask;
    uint_t Count;
    uint_t Last;
  };

  class StaticPattern
  {
  public:
    explicit StaticPattern(const Pattern& pat)
    {
      Data.reserve(pat.size());
      for (const auto& pred : pat)
      {
        Data.emplace_back(*pred);
      }
    }

    StaticPattern(const StaticPattern&) = delete;
    StaticPattern& operator=(const StaticPattern&) = delete;
    StaticPattern& operator=(StaticPattern&& rh) noexcept = default;
    StaticPattern(StaticPattern&& rh) noexcept = default;

    std::size_t GetSize() const
    {
      return Data.size();
    }

    const StaticPredicate& Get(std::size_t idx) const
    {
      return Data[idx];
    }

    // return back offsets of suffixes
    std::vector<std::size_t> GetSuffixOffsets() const;
    // return forward offset
    std::size_t FindPrefix(std::size_t prefixSize) const;

    bool Match(const uint8_t* blob) const
    {
      for (const auto& entry : Data)
      {
        if (!entry.Match(*blob))
        {
          return false;
        }
        ++blob;
      }
      return true;
    }

  private:
    const StaticPredicate* Begin() const
    {
      return Data.data();
    }

    const StaticPredicate* End() const
    {
      return Begin() + Data.size();
    }

    std::size_t FindMaxSuffixMatchSize(std::size_t offset) const;

  private:
    std::vector<StaticPredicate> Data;
  };
}  // namespace Binary::FormatDSL
