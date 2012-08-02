/*
Abstract:
  Network provider stub

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
//text includes
#include <io/text/io.h>

namespace ZXTune
{
  namespace IO
  {
    void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
    {
      enumerator.RegisterProvider(CreateDisabledProviderStub(Text::IO_NETWORK_PROVIDER_ID, Text::IO_NETWORK_PROVIDER_DESCRIPTION));
    }
  }
}
