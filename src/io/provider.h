/*
Abstract:
  Data provider interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_PROVIDER_H_DEFINED__
#define __IO_PROVIDER_H_DEFINED__

#include "container.h"

#include <error.h>

#include <boost/function.hpp>

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
      //flags
      uint32_t Flags;
      //custom parameters
      ParametersMap Parameters;
    };
  
    Error OpenData(const String& uri, const OpenDataParameters& params, DataContainer::Ptr& data, String& subpath);

    Error SplitUri(const String& uri, String& baseUri, String& subpath);

    Error CombineUri(const String& baseUri, const String& subpath, String& uri);

    struct ProviderInfo
    {
      ProviderInfo()
      {
      }
      ProviderInfo(const String& name, const String& descr) : Name(name), Description(descr)
      {
      }
      String Name;
      String Description;
    };
    
    void GetSupportedProviders(std::vector<ProviderInfo>& providers);
  }
}

#endif //__IO_PROVIDER_H_DEFINED__
