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

//local includes
#include "gates/curl_api.h"
#include "enumerator.h"

namespace IO
{
  DataProvider::Ptr CreateFileDataProvider();
  DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api);
}
