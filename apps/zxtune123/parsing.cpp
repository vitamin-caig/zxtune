#include "parsing.h"
#include "error_codes.h"

#include <error_tools.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define FILE_TAG 0DBA1FA8

namespace
{
  static const Char PARAMETERS_DELIMITERS[] = {';', '|', '\0'};
}

Error ParseParametersString(const String& pfx, const String& str, Parameters::Map& result)
{
  String prefix(pfx);
  if (!prefix.empty() && Parameters::NAMESPACE_DELIMITER != *prefix.rbegin())
  {
    prefix += Parameters::NAMESPACE_DELIMITER;
  }
  Parameters::Map res;
  StringArray splitted;
  boost::algorithm::split(splitted, str, boost::algorithm::is_any_of(PARAMETERS_DELIMITERS));
  for (StringArray::const_iterator pit = splitted.begin(), plim = splitted.end(); pit != plim; ++pit)
  {
    const String::size_type eqpos = pit->find_first_of('=');
    if (String::npos == eqpos)
    {
      return MakeFormattedError(THIS_LINE, INVALID_PARAMETER,
        "Invalid parameter '%1%' in parameter string '%2%'.", *pit, str);
    }
    res.insert(Parameters::Map::value_type(prefix + pit->substr(0, eqpos), Parameters::ConvertFromString(pit->substr(eqpos + 1))));
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
  (Formatter("%1%:%2$02:%3$02u.%4$02u") % hours % minutes % seconds % frames).str()
  :
  (Formatter("%1%:%2$02u.%3$02u") % minutes % seconds % frames).str();
}
