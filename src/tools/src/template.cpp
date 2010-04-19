/*
Abstract:
  String template working implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <template.h>

String InstantiateTemplate(const String& templ, const StringMap& properties,
  InstantiateMode mode, Char beginMark, Char endMark)
{
  bool inField(false);
  String field;
  String result;
  result.reserve(templ.size() * 2);//approx
  for (String::const_iterator it = templ.begin(), lim = templ.end(); it != lim; ++it)
  {
    if (*it == beginMark)
    {
      field.clear();
      inField = true;
      continue;
    }
    if (*it == endMark)
    {
      if (inField)
      {
        const StringMap::const_iterator propIt(properties.find(field));
        if (properties.end() != propIt)
        {
          result += propIt->second;
        }
        else if (KEEP_NONEXISTING == mode)
        {
          result += beginMark;
          result += field;
          result += *it;
        }
        else if (FILL_NONEXISTING == mode)
        {
          result += String(field.size() + 2, ' ');
        }
        inField = false;
      }
      continue;
    }
    (inField ? field : result) += *it;
  }
  if (inField)
  {
    result += beginMark;
    result += field;
  }
  return result;
}
