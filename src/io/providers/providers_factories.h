/*
Abstract:
  Providers factories (for testing purposes)

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef IO_PROVIDERS_FACTORIES_H_DEFINED
#define IO_PROVIDERS_FACTORIES_H_DEFINED

//local includes
#include "curl_api.h"
#include "enumerator.h"

namespace ZXTune
{
  namespace IO
  {
    DataProvider::Ptr CreateFileDataProvider();
    DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api);
  }
}

#endif //IO_PROVIDERS_FACTORIES_H_DEFINED
