/*
Abstract:
  Curl api gate interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef IO_CURL_API_H_DEFINED
#define IO_CURL_API_H_DEFINED

//platform-specific includes
#include <curl/curl.h>
#include <curl/easy.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace IO
  {
    namespace Curl
    {
      class Api
      {
      public:
        typedef boost::shared_ptr<Api> Ptr;
        virtual ~Api() {}

        
        virtual CURL *curl_easy_init() = 0;
        virtual void curl_easy_cleanup(CURL *curl) = 0;
        virtual CURLcode curl_easy_perform(CURL *curl) = 0;
        virtual const char *curl_easy_strerror(CURLcode errornum) = 0;
        virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, int intParam) = 0;
        virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* strParam) = 0;
        virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, void* opaqueParam) = 0;
        virtual CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, void* opaqueResult) = 0;
      };

      //throw exception in case of error
      Api::Ptr LoadDynamicApi();

    }
  }
}

#endif //IO_CURL_API_H_DEFINED
