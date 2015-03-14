/**
* 
* @file
*
* @brief  Parsing tools implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "config.h"
//common includes
#include <error_tools.h>
//library includes
#include <parameters/serialize.h>
#include <strings/map.h>
//std includes
#include <cctype>
#include <fstream>
//text includes
#include "text/text.h"

#define FILE_TAG 0DBA1FA8

namespace
{
  static const Char PARAMETERS_DELIMITER = ',';

  String GetDefaultConfigFileWindows()
  {
    if (const char* homeDir = ::getenv(ToStdString(Text::ENV_HOMEDIR_WIN).c_str()))
    {
      return FromStdString(homeDir) + '\\' + Text::CONFIG_PATH_WIN;
    }
    return Text::CONFIG_FILENAME;
  }

  String GetDefaultConfigFileNix()
  {
    const String configPath(Text::CONFIG_PATH_NIX);
    if (const char* homeDir = ::getenv(ToStdString(Text::ENV_HOMEDIR_NIX).c_str()))
    {
      return FromStdString(homeDir) + '/' + Text::CONFIG_PATH_NIX;
    }
    return Text::CONFIG_FILENAME;
  }

  //try to search config in homedir, if defined
  inline String GetDefaultConfigFile()
  {
#ifdef _WIN32
    return GetDefaultConfigFileWindows();
#else
    return GetDefaultConfigFileNix();
#endif
  }

  void ParseParametersString(const Parameters::NameType& prefix, const String& str, Strings::Map& result)
  {
    Strings::Map res;
  
    enum
    {
      IN_NAME,
      IN_VALUE,
      IN_VALSTR,
      IN_NOWHERE
    } mode = IN_NAME;

    /*
     parse strings in form name=value[,name=value...]
      name ::= [\w\d\._]*
      value ::= \"[^\"]*\"
      value ::= [^,]*
      name is prepended with prefix before insert to result
    */  
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
        else if (!std::isspace(sym))
        {
          paramName += sym;
        }
        else
        {
          throw MakeFormattedError(THIS_LINE, Text::ERROR_INVALID_FORMAT, str);
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
        res.insert(Strings::Map::value_type(FromStdString((prefix + ToStdString(paramName)).FullPath()), paramValue));
        paramName.clear();
        paramValue.clear();
      }
    }
    if (IN_VALUE == mode)
    {
      res.insert(Strings::Map::value_type(FromStdString((prefix + ToStdString(paramName)).FullPath()), paramValue));
    }
    else if (IN_NOWHERE != mode)
    {
      throw MakeFormattedError(THIS_LINE, Text::ERROR_INVALID_FORMAT, str);
    }
    result.swap(res);
  }

  void ParseConfigFile(const String& filename, String& params)
  {
    const String configName(filename.empty() ? Text::CONFIG_FILENAME : filename);

    typedef std::basic_ifstream<Char> FileStream;
    std::auto_ptr<FileStream> configFile(new FileStream(configName.c_str()));
    if (!*configFile)
    {
      if (!filename.empty())
      {
        throw Error(THIS_LINE, Text::ERROR_CONFIG_FILE);
      }
      configFile.reset(new FileStream(GetDefaultConfigFile().c_str()));
    }
    if (!*configFile)
    {
      params.clear();
      return;
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
            lines += PARAMETERS_DELIMITER;
          }
          lines += String(beginof, endof);
        }
      }
      else
      {
        break;
      }
    }
    params = lines;
  }
}

void ParseConfigFile(const String& filename, Parameters::Modifier& result)
{
  String strVal;
  ParseConfigFile(filename, strVal);
  if (!strVal.empty())
  {
    ParseParametersString(String(), strVal, result);
  }
}

void ParseParametersString(const Parameters::NameType& pfx, const String& str, Parameters::Modifier& result)
{
  Strings::Map strMap;
  ParseParametersString(pfx, str, strMap);
  Parameters::Convert(strMap, result);
}
