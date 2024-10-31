/**
 *
 * @file
 *
 * @brief  Parameters version tracking helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <types.h>

namespace Parameters
{
  template<class ParamsType>
  class TrackingHelper
  {
  public:
    explicit TrackingHelper(typename ParamsType::Ptr params)
      : Params(std::move(params))
      , Version(~uint_t(0))
    {}

    bool IsChanged() const
    {
      const uint_t newVers = Params->Version();
      if (newVers == Version)
      {
        return false;
      }
      else
      {
        Version = newVers;
        return true;
      }
    }

    void Reset()
    {
      Version = ~uint_t(0);
    }

    const ParamsType* operator->() const
    {
      return Params.get();
    }

    const ParamsType& operator*() const
    {
      return *Params;
    }

  private:
    const typename ParamsType::Ptr Params;
    mutable uint_t Version;
  };
}  // namespace Parameters
