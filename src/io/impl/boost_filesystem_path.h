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

// common includes
#include <types.h>
// boost includes
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace IO
{
  namespace Details
  {
    inline const boost::filesystem::path::codecvt_type& GetFacet()
    {
      static const boost::filesystem::detail::utf8_codecvt_facet instance;
      return instance;
    }

    inline String ToString(const boost::filesystem::path& path)
    {
      return path.string<String>(GetFacet());
    }

    inline boost::filesystem::path FromString(StringView str)
    {
      return boost::filesystem::path(str, GetFacet());
    }
  }  // namespace Details
}  // namespace IO
