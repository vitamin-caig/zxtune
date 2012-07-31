/*
Abstract:
  Supported providers list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __IO_PROVIDERS_LIST_H_DEFINED__
#define __IO_PROVIDERS_LIST_H_DEFINED__

namespace ZXTune
{
  namespace IO
  {
    class ProvidersEnumerator;

    //forward declarations of supported providers
    void RegisterFileProvider(ProvidersEnumerator& enumerator);
    void RegisterNetworkProvider(ProvidersEnumerator& enumerator);

    inline void RegisterProviders(ProvidersEnumerator& enumerator)
    {
      RegisterFileProvider(enumerator);
      RegisterNetworkProvider(enumerator);
    }
  }
}

#endif //__IO_PROVIDERS_LIST_H_DEFINED__
