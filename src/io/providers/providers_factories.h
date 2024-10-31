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

#include "enumerator.h"
#include "gates/curl_api.h"

namespace IO
{
  DataProvider::Ptr CreateFileDataProvider();
  DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api);
}  // namespace IO
