/*
Abstract:
  Parsing tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#include "parsing.h"
#include "error_codes.h"

#include <error_tools.h>

#include "text.h"

#define FILE_TAG 0DBA1FA8

namespace
{
  static const Char PARAMETERS_DELIMITER = ',';
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
          TEXT_ERROR_INVALID_FORMAT, str);
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
      TEXT_ERROR_INVALID_FORMAT, str);
  }
  result.swap(res);
  return Error();
}

String UnparseFrameTime(unsigned timeInFrames, unsigned frameDurationMicrosec)
{
  const unsigned fpsRough = 1000000u / frameDurationMicrosec;
  const unsigned allSeconds = static_cast<unsigned>(uint64_t(timeInFrames) * frameDurationMicrosec / 1000000u);
  const unsigned frames = timeInFrames % fpsRough;
  const unsigned seconds = allSeconds % 60;
  const unsigned minutes = (allSeconds / 60) % 60;
  const unsigned hours = allSeconds / 3600;
  return hours ?
  (Formatter(TEXT_TIME_FORMAT_HOURS) % hours % minutes % seconds % frames).str()
  :
  (Formatter(TEXT_TIME_FORMAT) % minutes % seconds % frames).str();
}
