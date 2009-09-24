/*
Abstract:
  Data provider interface

Last changed:
  $Id: $

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_PROVIDER_H_DEFINED__
#define __IO_PROVIDER_H_DEFINED__

#include <io/container.h>

#include <error.h>

#include <boost/functions.h>

#include <vector>

namespace ZXTune
{
  namespace IO
  {
    enum
    {
    //disk-related flags
      //open files using memory mapping
      USE_MMAP   = 0x00000001,
    //network-related flags
      //disable caching
      NO_CACHE   = 0x00010000,
      //use only cache
      ONLY_CACHE = 0x00020000,
    };
  
    struct OpenDataParameters
    {
      OpenDataParameters() : Flags()
      {
      }
    
      //progress callback. In case if result Error is not success, it used as a suberror of 'cancelled' error
      boost::function<Error(const String&, unsigned)> ProgressCallback;
      //credentials callback
      boost::function<Error(const String&, String&, String&)> CredentialsCallback;
      //flags
      uint32_t Flags;
      //custom parameters
      StringMap Parameters;
    };
  
    Error OpenData(const String& uri, const OpenDataParameters& params, DataContainer::Ptr& data, String& subpath);
    
    Error SplitUri(const String& uri, String& baseUri, String& subpath);
    
    struct ProviderInfo
    {
      String Name;
      String Description;
    };
    
    void GetSupportedProviders(std::vector<ProviderInfo>& providers);
  }
}

#endif //__IO_PROVIDER_H_DEFINED__
