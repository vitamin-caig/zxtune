/**
 *
 * @file
 *
 * @brief  Network provider stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "io/providers/enumerator.h"
#include "io/providers/network_provider.h"
// common includes
#include <l10n/api.h>

namespace IO
{
  void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
  {
    enumerator.RegisterProvider(CreateDisabledProviderStub(
        Network::PROVIDER_IDENTIFIER, L10n::translate("Network files access via different schemes support")));
  }
}  // namespace IO
