/**
*
* @file
*
* @brief  Static token helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "expression.h"
//common includes
#include <contract.h>
//std includes
#include <vector>
//boost includes
#include <boost/array.hpp>

namespace Binary
{
  class StaticToken
  {
  public:
    explicit StaticToken(Token::Ptr tok)
      : Mask()
      , Count()
      , Last()
    {
      for (uint_t idx = 0; idx != 256; ++idx)
      {
        if (tok->Match(idx))
        {
          Set(idx);
        }
      }
    }

    explicit StaticToken(uint_t val)
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
      return Count == 1
        ? &Last
        : 0;
    }

    static bool AreIntersected(const StaticToken& lh, const StaticToken& rh)
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
    typedef uint_t ElementType;
    static const std::size_t BitsPerElement = 8 * sizeof(ElementType);
    static const std::size_t ElementsCount = 256 / BitsPerElement;
    boost::array<ElementType, ElementsCount> Mask;
    uint_t Count;
    uint_t Last;
  };

  class StaticPattern
  {
  public:
    explicit StaticPattern(ObjectIterator<Token::Ptr>::Ptr iter)
    {
      for (; iter->IsValid(); iter->Next())
      {
        Data.push_back(StaticToken(iter->Get()));
      }
    }

    std::size_t GetSize() const
    {
      return Data.size();
    }

    const StaticToken& Get(std::size_t idx) const
    {
      return Data[idx];
    }

    //return back offset
    std::size_t FindSuffix(std::size_t suffixSize) const;
    //return forward offset
    std::size_t FindPrefix(std::size_t prefixSize) const;
  private:
    const StaticToken* Begin() const
    {
      return &Data.front();
    }

    const StaticToken* End() const
    {
      return &Data.back() + 1;
    }
  private:
    std::vector<StaticToken> Data;
  };
}
