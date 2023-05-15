/**
 *
 * @file
 *
 * @brief  Curl api gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-specific includes
#include <3rdparty/curl/curl.h>
#include <3rdparty/curl/easy.h>

namespace IO
{
  namespace Curl
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

// clang-format off

      virtual char* curl_version() = 0;
      virtual CURL *curl_easy_init() = 0;
      virtual void curl_easy_cleanup(CURL *curl) = 0;
      virtual CURLcode curl_easy_perform(CURL *curl) = 0;
      virtual const char *curl_easy_strerror(CURLcode errornum) = 0;
      virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, int intParam) = 0;
      virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* strParam) = 0;
      virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, void* opaqueParam) = 0;
      virtual CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, void* opaqueResult) = 0;
// clang-format on
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();
  }
}  // namespace IO
