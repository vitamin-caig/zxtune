/**
*
* @file
*
* @brief  Data location interace and factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <analysis/path.h>
#include <binary/container.h>
#include <parameters/accessor.h>

namespace ZXTune
{
  // Describes piece of data and defenitely location inside
  class DataLocation
  {
  public:
    typedef boost::shared_ptr<const DataLocation> Ptr;
    virtual ~DataLocation() {}

    virtual Binary::Container::Ptr GetData() const = 0;
    virtual Analysis::Path::Ptr GetPath() const = 0;
    virtual Analysis::Path::Ptr GetPluginsChain() const = 0;
  };

  //! @param coreParams Parameters for plugins processing
  //! @param data Source data to be processed
  //! @param subpath Subpath in source data to be resolved
  //! @return Object if path is valid. No object elsewhere
  DataLocation::Ptr CreateLocation(Binary::Container::Ptr data);
  DataLocation::Ptr OpenLocation(Parameters::Accessor::Ptr coreParams, Binary::Container::Ptr data, const String& subpath);
}
