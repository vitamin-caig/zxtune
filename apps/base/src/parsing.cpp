/*
Abstract:
  Parsing tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <apps/base/parsing.h>
#include <apps/base/error_codes.h>

#include <error_tools.h>
#include <io/fs_tools.h>

#include <cctype>

#include "../text/base_text.h"

#define FILE_TAG 0DBA1FA8

namespace
{
  static const Char PARAMETERS_DELIMITER = ',';

  inline String GetDefaultConfigFile()
  {
#ifdef _WIN32
    const String configPath(Text::CONFIG_PATH_WIN);
    if (const char* homeDir = ::getenv(ToStdString(Text::ENV_HOMEDIR_WIN).c_str()))
#else
    const String configPath(Text::CONFIG_PATH_NIX);
    if (const char* homeDir = ::getenv(ToStdString(Text::ENV_HOMEDIR_NIX).c_str()))
#endif
    {
      return ZXTune::IO::AppendPath(FromStdString(homeDir), configPath);
    }
    return Text::CONFIG_FILENAME;
  }
}

Error ParseConfigFile(const String& filename, Parameters::Map& params)
{
  const String configName(filename.empty() ? Text::CONFIG_FILENAME : filename);

  typedef std::basic_ifstream<Char> FileStream;
  std::auto_ptr<FileStream> configFile(new FileStream(configName.c_str()));
  if (!*configFile)
  {
    if (!filename.empty())
    {
      return Error(THIS_LINE, CONFIG_FILE, Text::ERROR_CONFIG_FILE);
    }
    configFile.reset(new FileStream(GetDefaultConfigFile().c_str()));
  }
  if (!*configFile)
  {
    params.clear();
    return Error();
  }

  String lines;
  std::vector<Char> buffer(1024);
  for (;;)
  {
    configFile->getline(&buffer[0], buffer.size());
    if (const std::streamsize lineSize = configFile->gcount())
    {
      std::vector<Char>::const_iterator endof(buffer.begin() + lineSize - 1);
      std::vector<Char>::const_iterator beginof(std::find_if<std::vector<Char>::const_iterator>(buffer.begin(), endof,
      std::not1(std::ptr_fun<int, int>(&std::isspace))));
      if (beginof != endof && *beginof != Char('#'))
      {
        if (!lines.empty())
        {
          lines += Char(',');
        }
        lines += String(beginof, endof);
      }
    }
    else
    {
      break;
    }
  }
  if (lines.empty())
  {
    params.clear();
  }
  else if (const Error& e = ParseParametersString(String(), lines, params))
  {
    return e;
  }
  return Error();
}

Error ParseParametersString(const String& pfx, const String& str, Parameters::Map& result)
{
  String prefix(pfx);
  if (!prefix.empty() && Parameters::NAMESPACE_DELIMITER != *prefix.rbegin())
  {
    prefix += Parameters::NAMESPACE_DELIMITER;
  }
  Parameters::Map res;
  
  enum
  {
    IN_NAME,
    IN_VALUE,
    IN_VALSTR,
    IN_NOWHERE
  } mode = IN_NAME;
  
  String paramName, paramValue;
  for (String::const_iterator it = str.begin(), lim = str.end(); it != lim; ++it)
  {
    bool doApply = false;
    const Char sym(*it);
    switch (mode)
    {
    case IN_NOWHERE:
      if (sym == PARAMETERS_DELIMITER)
      {
        break;
      }
    case IN_NAME:
      if (sym == '=')
      {
        mode = IN_VALUE;
      }
      else if (std::isalnum(sym) || Parameters::NAMESPACE_DELIMITER == sym || sym == '_')
      {
        paramName += sym;
      }
      else
      {
        return MakeFormattedError(THIS_LINE, INVALID_PARAMETER,
          Text::ERROR_INVALID_FORMAT, str);
      }
      break;
    case IN_VALUE:
      if (Parameters::STRING_QUOTE == sym)
      {
        paramValue += sym;
        mode = IN_VALSTR;
      }
      else if (sym == PARAMETERS_DELIMITER)
      {
        doApply = true;
        mode = IN_NOWHERE;
        --it;
      }
      else
      {
        paramValue += sym;
      }
      break;
    case IN_VALSTR:
      paramValue += sym;
      if (Parameters::STRING_QUOTE == sym)
      {
        doApply = true;
        mode = IN_NOWHERE;
      }
      break;
    default:
      assert(!"Invalid state");
    };

    if (doApply)
    {
      res.insert(Parameters::Map::value_type(prefix + paramName, Parameters::ConvertFromString(paramValue)));
      paramName.clear();
      paramValue.clear();
    }
  }
  if (IN_VALUE == mode)
  {
    res.insert(Parameters::Map::value_type(prefix + paramName, Parameters::ConvertFromString(paramValue)));
  }
  else if (IN_NOWHERE != mode)
  {
    return MakeFormattedError(THIS_LINE, INVALID_PARAMETER,
      Text::ERROR_INVALID_FORMAT, str);
  }
  result.swap(res);
  return Error();
}
