/**
*
* @file
*
* @brief  boost::filesystem adapters
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//boost includes
#include <boost/filesystem/path.hpp>

namespace IO
{
  namespace Details
  {
    //to distinguish difference between boost::filesystem v2 and v3
    inline boost::filesystem::path::string_type ToString(const boost::filesystem::path::string_type& str)
    {
      return str;
    }
  
    inline String ToString(const boost::filesystem::path& path)
    {
      return path.string();
    }
  }
}
