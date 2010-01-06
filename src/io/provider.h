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

#include <error.h>
#include <parameters_types.h>

#include <io/container.h>

#include <boost/function.hpp>

#include <vector>

namespace ZXTune
{
  namespace IO
  {
      //progress callback. In case if result Error is not success, it used as a suberror of 'cancelled' error
    typedef boost::function<Error(const String&, unsigned)> ProgressCallback;
    Error OpenData(const String& uri, const Parameters::Map& params, const ProgressCallback& cb, DataContainer::Ptr& data, String& subpath);

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
