/*
Abstract:
  boost::filesystem adapters

Last changed:
  $Id: template.cpp 2069 2012-10-22 08:18:03Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef IO_BOOST_FILESYSTEM_PATH_H_DEFINED
#define IO_BOOST_FILESYSTEM_PATH_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/filesystem/path.hpp>

namespace IO
{
  namespace Details
  {
    //to distinguish difference between boost::filesystem v2 and v3
    inline String ToString(const boost::filesystem::path::string_type& str)
    {
      return str;
    }
  
    inline String ToString(const boost::filesystem::path& path)
    {
      return path.string();
    }
  }
}

#endif //IO_BOOST_FILESYSTEM_PATH_H_DEFINED
