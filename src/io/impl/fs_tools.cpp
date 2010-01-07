/*
Abstract:
  Different io-related tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <io/fs_tools.h>

namespace
{
  const Char FS_DELIMITER = '/';
}

namespace ZXTune
{
  namespace IO
  {
    String ExtractFirstPathComponent(const String& path, String& restPart)
    {
      const String::size_type delimPos(path.find_first_of(FS_DELIMITER));
      if (String::npos == delimPos)
      {
        restPart.clear();
        return path;
      }
      else
      {
        restPart = path.substr(delimPos + 1);
        return path.substr(0, delimPos);
      }
    }

    String AppendPath(const String& path1, const String& path2)
    {
      String result(path1);
      if (!path1.empty() && *path1.rbegin() != FS_DELIMITER &&
          !path2.empty() && *path2.begin() != FS_DELIMITER)
      {
        result += FS_DELIMITER;
      }
      result += path2;
      return result;
    }
  }
}
