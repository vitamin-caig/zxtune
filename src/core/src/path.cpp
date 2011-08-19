/*
Abstract:
  Data path implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "path.h"
//common includes
#include <string_helpers.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
//std includes
#include <cassert>
//texts
#include <core/text/core.h>

namespace
{
  using namespace ZXTune;

  const String DELIMITER(Text::MODULE_SUBPATH_DELIMITER);

  void SplitPath(const String& str, StringArray& res)
  {
    StringArray parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(DELIMITER), boost::algorithm::token_compress_on);
    res.swap(parts);
  }

  class DataPathImpl : public DataPath
  {
  public:
    DataPathImpl(StringArray::const_iterator from, StringArray::const_iterator to)
      : Components(from, to)
    {
      assert(!Components.empty());
    }

    virtual String AsString() const
    {
      return boost::algorithm::join(Components, DELIMITER);
    }

    virtual String GetFirstComponent() const
    {
      return Components.front();
    }
  private:
    const StringArray Components;
  };

  class MergedDataPath : public DataPath
  {
  public:
    MergedDataPath(Ptr delegate, const String& component)
      : Delegate(delegate)
      , Component(component)
    {
    }

    virtual String AsString() const
    {
      const String base = Delegate->AsString();
      if (base.empty())
      {
        return Component;
      }
      else
      {
        return base + DELIMITER + Component;
      }
    }

    virtual String GetFirstComponent() const
    {
      return Delegate->GetFirstComponent();
    }
  private:
    const Ptr Delegate;
    const String Component;
  };
}

namespace ZXTune
{
  DataPath::Ptr CreateDataPath(const String& str)
  {
    StringArray splitted;
    SplitPath(str, splitted);
    if (splitted.empty())
    {
      return DataPath::Ptr();
    }
    else
    {
      return boost::make_shared<DataPathImpl>(splitted.begin(), splitted.end());
    }
  }

  DataPath::Ptr CreateMergedDataPath(DataPath::Ptr lh, const String& component)
  {
    return boost::make_shared<MergedDataPath>(lh, component);
  }

  DataPath::Ptr SubstractDataPath(const DataPath& lh, const DataPath& rh)
  {
    const String fullPath = lh.AsString();
    const String subPath = rh.AsString();
    StringArray fullSplitted, subSplitted;
    SplitPath(fullPath, fullSplitted);
    SplitPath(subPath, subSplitted);
    if (subSplitted.size() >= fullSplitted.size())
    {
      return DataPath::Ptr();
    }
    const std::pair<StringArray::const_iterator, StringArray::const_iterator> iters =
      std::mismatch(subSplitted.begin(), subSplitted.end(), fullSplitted.begin());
    if (iters.first != subSplitted.end())
    {
      return DataPath::Ptr();
    }
    return boost::make_shared<DataPathImpl>(iters.second, fullSplitted.end());
  }
}
