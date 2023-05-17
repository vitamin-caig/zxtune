/**
 *
 * @file
 *
 * @brief  Simple order list implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/track_model.h"
// std includes
#include <vector>

namespace Module
{
  class SimpleOrderList : public OrderList
  {
  public:
    SimpleOrderList(uint_t loop, std::vector<uint_t> order)
      : Loop(loop)
      , Order(std::move(order))
    {}

    uint_t GetSize() const override
    {
      return static_cast<uint_t>(Order.size());
    }

    uint_t GetPatternIndex(uint_t pos) const override
    {
      return Order[pos];
    }

    uint_t GetLoopPosition() const override
    {
      return Loop;
    }

  private:
    const uint_t Loop;
    const std::vector<uint_t> Order;
  };

  template<class T>
  class SimpleOrderListWithTransposition : public OrderList
  {
  public:
    using Ptr = std::unique_ptr<const SimpleOrderListWithTransposition<T>>;

    SimpleOrderListWithTransposition(uint_t loop, std::vector<T> positions)
      : Loop(loop)
      , Positions(std::move(positions))
    {}

    explicit SimpleOrderListWithTransposition(std::vector<T> positions)
      : Loop(0)
      , Positions(std::move(positions))
    {}

    uint_t GetSize() const override
    {
      return static_cast<uint_t>(Positions.size());
    }

    uint_t GetPatternIndex(uint_t pos) const override
    {
      return Positions[pos].PatternIndex;
    }

    uint_t GetLoopPosition() const override
    {
      return Loop;
    }

    int_t GetTransposition(uint_t pos) const
    {
      return Positions[pos].Transposition;
    }

  private:
    const uint_t Loop;
    const std::vector<T> Positions;
  };
}  // namespace Module
