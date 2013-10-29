/**
*
* @file
*
* @brief  Supported providers list
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
