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

//common includes
#include <types.h>

namespace Devices
{
  namespace Details
  {
    template<class ParamsType>
    class ParametersHelper
    {
    public:
      explicit ParametersHelper(typename ParamsType::Ptr params)
        : Params(params)
        , Version(~uint_t(0))
      {
      }

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

      const ParamsType* operator -> () const
      {
        return Params.get();
      }

      const ParamsType& operator * () const
      {
        return *Params;
      }
    private:
      const typename ParamsType::Ptr Params;
      mutable uint_t Version;
    };
  }
}
