#ifndef __LOCATION_H_DEFINED__
#define __LOCATION_H_DEFINED__

#include <boost/algorithm/string.hpp>

#include <functional>

namespace ZXTune
{
  namespace IO
  {
    /*
      Location-related functions

      LOCATION ::= FS_PATH
      LOCATION ::= LOCATION SUBPATH
      FS_PATH ::= ROOT
      FS_PATH ::= PATH / PATH
      SUBPATH ::= ?FS_PATH
      ROOT ::= / *:\

      E.g.:  /tmp/archive.zip?nested/path/in/archive/container.trd?archive1.hrp?module.ext
             |<---FS_PATH--->|<-------------SUBPATH1------------->|<-SUBPATH2->|<-SUBPATH3
   */
    const String::value_type SUBPATH_DELIMITER = '\?';

    inline String ExtractFSPath(const String& path)
    {
      return path.substr(0, path.find(SUBPATH_DELIMITER));
    }

    inline String ExtractSubpath(const String& path)
    {
      String::size_type pos(path.find(SUBPATH_DELIMITER));
      return String::npos == pos ? String() : path.substr(pos + 1);
    }

    inline void SplitPath(const String& path, StringArray& result)
    {
      boost::algorithm::split(result, path, 
        std::bind1st(std::equal_to<String::value_type>(), SUBPATH_DELIMITER));
    }

    inline String CombinePath(StringArray::const_iterator begin, StringArray::const_iterator end)
    {
      String result;
      for (; begin != end; ++begin)
      {
        if (!result.empty())
        {
          result += SUBPATH_DELIMITER;
        }
        result += *begin;
      }
      return result;
    }

    inline String CombinePath(const StringArray& subpathes)
    {
      return CombinePath(subpathes.begin(), subpathes.end());
    }

    inline String CombinePath(const String& lh, const String& rh)
    {
      return lh + SUBPATH_DELIMITER + rh;
    }
  }
}

#endif //__LOCATION_H_DEFINED__
