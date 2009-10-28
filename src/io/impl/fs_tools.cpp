/*
Abstract:
  Different io-related tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../fs_tools.h"

namespace
{
  const String::value_type TEMPLATE_BEGIN = '[';
  const String::value_type TEMPLATE_END = ']';

  const String::value_type FS_DELIMITER = '/';
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
    
    /*
    void SplitFSName(const String& fullpath, String& dir, String& filename, String& subname)
    {
      StringArray parts;
      SplitPath(fullpath, parts);
      const String& fspath(parts.front());
      const String::size_type slpos(fspath.find_last_of(FS_DELIMITERS));
      dir = fspath.substr(0, String::npos == slpos ? 0 : slpos);
      filename = fspath.substr(String::npos == slpos ? 0 : slpos + 1);
      subname = 1 == parts.size() ? String() : parts.back();
    }

    String BuildNameTemplate(const String& fullpath, const String& templ, const StringMap& properties)
    {
      String dir, filename, subname;
      SplitFSName(fullpath, dir, filename, subname);
      bool inField(false);
      String field;
      String result;
      result.reserve(templ.size() * 2);//approx
      for (String::const_iterator it = templ.begin(), lim = templ.end(); it != lim; ++it)
      {
        if (*it == TEMPLATE_BEGIN)
        {
          field.clear();
          inField = true;
          continue;
        }
        if (*it == TEMPLATE_END)
        {
          if (inField)
          {
            if (field == TEXT_TEMPLATE_FIELD_CONTAINER)
            {
              result += filename;
            }
            else if (field == TEXT_TEMPLATE_FIELD_NAME)
            {
              result += subname.empty() ? filename : subname;
            }
            else
            {
              StringMap::const_iterator propIt(properties.find(field));
              if (properties.end() != propIt)
              {
                result += propIt->second;
              }
              else
              {
                result += TEMPLATE_BEGIN;
                result += field;
                result += *it;
              }
            }
            inField = false;
          }
          continue;
        }
        (inField ? field : result) += *it;
      }
      return result;
    }
    */
  }
}
