/**
 *
 * @file
 *
 * @brief  Providers factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "gates/curl_api.h"

#include "io/providers/enumerator.h"

namespace IO
{
  DataProvider::Ptr CreateFileDataProvider();
  DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api);
}  // namespace IO
