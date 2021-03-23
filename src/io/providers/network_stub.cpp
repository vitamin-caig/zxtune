/**
 *
 * @file
 *
 * @brief  Network provider stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/providers/enumerator.h"
#include "io/providers/network_provider.h"

namespace IO
{
  void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
  {
    enumerator.RegisterProvider(
        CreateDisabledProviderStub(Network::PROVIDER_IDENTIFIER, Network::PROVIDER_DESCRIPTION));
  }
}  // namespace IO
