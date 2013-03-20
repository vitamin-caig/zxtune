/*
Abstract:
  Simple order list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_SIMPLE_ORDERLIST_H_DEFINED
#define CORE_PLUGINS_PLAYERS_SIMPLE_ORDERLIST_H_DEFINED

//local includes
#include "track_model.h"
//std includes
#include <vector>

namespace ZXTune
{
  namespace Module
  {
    class SimpleOrderList : public OrderList
    {
    public:
      template<class It>
      SimpleOrderList(uint_t loop, It from, It to)
        : Loop(loop)
        , Order(from, to)
      {
      }

      template<class It>
      SimpleOrderList(It from, It to)
        : Loop(0)
        , Order(from, to)
      {
      }

      virtual uint_t GetSize() const
      {
        return static_cast<uint_t>(Order.size());
      }

      virtual uint_t GetPatternIndex(uint_t pos) const
      {
        return Order[pos];
      }

      virtual uint_t GetLoopPosition() const
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
      typedef boost::shared_ptr<const SimpleOrderListWithTransposition<T> > Ptr;

      template<class It>
      SimpleOrderListWithTransposition(uint_t loop, It from, It to)
        : Loop(loop)
        , Positions(from, to)
      {
      }

      template<class It>
      SimpleOrderListWithTransposition(It from, It to)
        : Loop(0)
        , Positions(from, to)
      {
      }

      virtual uint_t GetSize() const
      {
        return static_cast<uint_t>(Positions.size());
      }

      virtual uint_t GetPatternIndex(uint_t pos) const
      {
        return Positions[pos].PatternIndex;
      }

      virtual uint_t GetLoopPosition() const
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
  }
}

#endif //CORE_PLUGINS_PLAYERS_SIMPLE_ORDERLIST_H_DEFINED
