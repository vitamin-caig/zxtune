/*
Abstract:
  Different io-related tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <error_tools.h>
#include <io/error_codes.h>
#include <io/fs_tools.h>

#include <cctype>

#include <text/io.h>

#define FILE_TAG 9AC2A0AC

namespace
{
  const Char FS_DELIMITER = '/';

  inline bool IsFSSymbol(Char sym)
  {
    return std::isalnum(sym) || sym == '_' || sym =='(' || sym == ')';
  }
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

    String MakePathFromString(const String& input, Char replacing)
    {
      String result;
      for (String::const_iterator it = input.begin(), lim = input.end(); it != lim; ++it)
      {
        if (IsFSSymbol(*it))
        {
          result += *it;
        }
        else if (replacing != '\0' && !result.empty())
        {
          result += replacing;
        }
      }
      return replacing != '\0' ? result.substr(0, 1 + result.find_last_not_of(replacing)) : result;
    }

    std::auto_ptr<std::ofstream> CreateFile(const String& path, bool overwrite)
    {
      if (!overwrite && std::ifstream(path.c_str()))
      {
        throw MakeFormattedError(THIS_LINE, FILE_EXISTS, TEXT_IO_ERROR_FILE_EXISTS, path);
      }
      std::auto_ptr<std::ofstream> res(new std::ofstream(path.c_str(), std::ios::binary));
      if (res.get() && res->good())
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, NOT_FOUND, TEXT_IO_ERROR_NOT_OPENED, path);
    }
  }
}
