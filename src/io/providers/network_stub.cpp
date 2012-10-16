/*
Abstract:
  Network provider stub

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
//common includes
#include <l10n/api.h>
//text includes
#include <io/text/io.h>

namespace IO
{
  void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
  {
    enumerator.RegisterProvider(CreateDisabledProviderStub(Text::IO_NETWORK_PROVIDER_ID, L10n::translate("Network files access via different schemes support")));
  }
}
