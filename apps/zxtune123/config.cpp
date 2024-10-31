/**
 *
 * @file
 *
 * @brief  Parsing tools implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune123/config.h"

#include "parameters/modifier.h"
#include "parameters/serialize.h"
#include "parameters/types.h"
#include "strings/map.h"

#include "error_tools.h"
#include "string_view.h"

#include <stdlib.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <memory>

namespace
{
  const auto PARAMETERS_DELIMITER = ',';

  const auto CONFIG_FILENAME = "zxtune.conf"sv;

  // try to search config in homedir, if defined
  String GetDefaultConfigFile()
  {
#ifdef _WIN32
    static const char ENV_HOMEDIR[] = "APPDATA";
    const auto PATH = "\\zxtune\\"sv;
#else
    static const char ENV_HOMEDIR[] = "HOME";
    const auto PATH = "/.zxtune/"sv;
#endif
    String dir;
    if (const auto* homeDir = ::getenv(ENV_HOMEDIR))
    {
      dir = String(homeDir) + PATH;
    }
    return dir + CONFIG_FILENAME;
  }

  void ParseParametersString(Parameters::Identifier prefix, StringView str, Strings::Map& result)
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
    String paramName;
    String paramValue;
    for (auto it = str.begin(), lim = str.end(); it != lim; ++it)
    {
      bool doApply = false;
      const auto sym(*it);
      switch (mode)
      {
      case IN_NOWHERE:
        if (sym == PARAMETERS_DELIMITER)
        {
          break;
        }
        [[fallthrough]];
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
          throw MakeFormattedError(THIS_LINE, "Invalid parameter format '{}'.", str);
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
        res.emplace(prefix.Append(paramName), paramValue);
        paramName.clear();
        paramValue.clear();
      }
    }
    if (IN_VALUE == mode)
    {
      res.emplace(prefix.Append(paramName), paramValue);
    }
    else if (IN_NOWHERE != mode)
    {
      throw MakeFormattedError(THIS_LINE, "Invalid parameter format '{}'.", str);
    }
    result.swap(res);
  }

  void ParseConfigFile(StringView filename, String& params)
  {
    const String configName(filename.empty() ? CONFIG_FILENAME : filename);

    using FileStream = std::ifstream;
    std::unique_ptr<FileStream> configFile(new FileStream(configName.c_str()));
    if (!*configFile)
    {
      if (!filename.empty())
      {
        throw Error(THIS_LINE, "Failed to open configuration file " + configName);
      }
      configFile = std::make_unique<FileStream>(GetDefaultConfigFile().c_str());
    }
    if (!*configFile)
    {
      params.clear();
      return;
    }

    String lines;
    std::string buffer(1024, '\0');
    for (;;)
    {
      configFile->getline(buffer.data(), buffer.size());
      if (const std::streamsize lineSize = configFile->gcount())
      {
        const auto endof = buffer.cbegin() + lineSize - 1;
        const auto beginof = std::find_if(buffer.cbegin(), endof, [](auto c) { return !std::isspace(c); });
        if (beginof != endof && *beginof != '#')
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
}  // namespace

void ParseConfigFile(StringView filename, Parameters::Modifier& result)
{
  String strVal;
  ParseConfigFile(filename, strVal);
  if (!strVal.empty())
  {
    ParseParametersString("", strVal, result);
  }
}

void ParseParametersString(Parameters::Identifier prefix, StringView str, Parameters::Modifier& result)
{
  Strings::Map strMap;
  ParseParametersString(prefix, str, strMap);
  Parameters::Convert(strMap, result);
}
