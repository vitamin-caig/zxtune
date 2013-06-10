/*
Abstract:
  Parameters version tracking helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DETAILS_VERSION_HELPER_H_DEFINED
#define DEVICES_DETAILS_VERSION_HELPER_H_DEFINED

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
    private:
      const typename ParamsType::Ptr Params;
      mutable uint_t Version;
    };
  }
}

#endif //DEVICES_DETAILS_VERSION_HELPER_H_DEFINED
