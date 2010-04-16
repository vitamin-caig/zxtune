/**
*
* @file      io/provider.h
* @brief     Data provider interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __IO_PROVIDER_H_DEFINED__
#define __IO_PROVIDER_H_DEFINED__

// for Parameters::Map
#include <parameters.h>
// for IO::DataContainer::Ptr
#include <io/container.h>

#include <boost/function.hpp>

#include <vector>

//forward declarations
class Error;

namespace ZXTune
{
  namespace IO
  {
    //! @brief Progress callback. In case if result Error is not success, it used as a suberror of #ERROR_CANCELED error
    typedef boost::function<Error(const String&, uint_t)> ProgressCallback;
    
    //! @brief Performs opening specified uri
    //! @param uri Full path including external data identifier and raw data subpath delimited by scheme-specific delimiter
    //! @param params %Parameters map to setup providers work
    //! @param cb Callback for long-time controlable operations
    //! @param data Reference to result data container
    //! @param subpath Rest part of the uri which specifies path inside data dump
    //! @return Error() in case of success
    Error OpenData(const String& uri, const Parameters::Map& params, const ProgressCallback& cb, DataContainer::Ptr& data, String& subpath);

    //! @brief Performs splitting specified uri to filesystem and internal parts
    //! @param uri Full path
    //! @param baseUri Reference to external identifier result
    //! @param subpath Reference to internal identifier result
    //! @return Error() in case of success
    Error SplitUri(const String& uri, String& baseUri, String& subpath);

    //! @brief Performs combining external and internal identifiers in scheme-dependent style
    //! @param baseUri External data identifier
    //! @param subpath Internal data identifier
    //! @param uri Reference to merged uri result
    //! @return Error() in case of success
    Error CombineUri(const String& baseUri, const String& subpath, String& uri);

    //! @brief Structure describing supported %IO data provider
    struct ProviderInformation
    {
      //! Provider's name
      String Name;
      //! Description in any form
      String Description;
      //! Version in text form
      String Version;
    };
    
    //! @brief Set of the provider information structures
    typedef std::vector<ProviderInformation> ProviderInformationArray;
    
    //! @brief Enumerating supported %IO providers
    //! @param providers Reference to result set
    void EnumerateProviders(ProviderInformationArray& providers);
  }
}

#endif //__IO_PROVIDER_H_DEFINED__
