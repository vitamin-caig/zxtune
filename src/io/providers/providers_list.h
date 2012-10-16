/*
Abstract:
  Supported providers list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef IO_PROVIDERS_LIST_H_DEFINED
#define IO_PROVIDERS_LIST_H_DEFINED

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

#endif //IO_PROVIDERS_LIST_H_DEFINED
