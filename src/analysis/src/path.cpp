/**
 *
 * @file
 *
 * @brief Analyzed data path implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <analysis/path.h>
#include <strings/array.h>
#include <strings/split.h>
// boost includes
#include <boost/algorithm/string/join.hpp>
// std includes
#include <functional>

namespace Analysis
{
  Strings::Array SplitPath(StringView str, Char separator)
  {
    Strings::Array parts;
    Strings::Split(str, separator, parts);
    const auto newEnd = std::remove_if(parts.begin(), parts.end(), std::mem_fn(&String::empty));
    parts.erase(newEnd, parts.end());
    return parts;
  }

  String JoinPath(const Strings::Array& arr, Char separator)
  {
    const String delimiter(1, separator);
    return boost::algorithm::join(arr, delimiter);
  }

  template<class It>
  Path::Ptr CreatePath(Char separator, It from, It to);

  Path::Ptr CreatePath(Char separator, Strings::Array data);

  class ParsedPath : public Path
  {
  public:
    template<class Iter>
    ParsedPath(Char separator, Iter from, Iter to)
      : Components(from, to)
      , Separator(separator)
    {
      Require(from != to);
    }

    ParsedPath(Char separator, Strings::Array data)
      : Components(std::move(data))
      , Separator(separator)
    {
      Require(!Components.empty());
    }

    bool Empty() const override
    {
      return false;
    }

    String AsString() const override
    {
      return JoinPath(Components, Separator);
    }

    Iterator::Ptr GetIterator() const override
    {
      return CreateRangedObjectIteratorAdapter(Components.begin(), Components.end());
    }

    Ptr Append(const String& element) const override
    {
      const Strings::Array& newOne = SplitPath(element, Separator);
      Strings::Array result(Components.size() + newOne.size());
      std::copy(newOne.begin(), newOne.end(), std::copy(Components.begin(), Components.end(), result.begin()));
      return CreatePath(Separator, std::move(result));
    }

    Ptr Extract(const String& startPath) const override
    {
      const auto& subSplitted = SplitPath(startPath, Separator);
      if (subSplitted.size() > Components.size())
      {
        return Ptr();
      }
      const auto iters = std::mismatch(subSplitted.begin(), subSplitted.end(), Components.begin());
      if (iters.first != subSplitted.end())
      {
        return Ptr();
      }
      return CreatePath(Separator, iters.second, Components.end());
    }

    Ptr GetParent() const override
    {
      const auto first = Components.begin();
      auto last = Components.end();
      if (first != last)
      {
        --last;
      }
      return CreatePath(Separator, first, last);
    }

  private:
    const Strings::Array Components;
    const Char Separator;
  };

  class EmptyPath : public Path
  {
  public:
    explicit EmptyPath(Char separator)
      : Separator(separator)
    {}

    bool Empty() const override
    {
      return true;
    }

    String AsString() const override
    {
      return String();
    }

    Iterator::Ptr GetIterator() const override
    {
      return Iterator::CreateStub();
    }

    Ptr Append(const String& element) const override
    {
      return ParsePath(element, Separator);
    }

    Ptr Extract(const String& startPath) const override
    {
      return startPath.empty() ? MakePtr<EmptyPath>(Separator) : Ptr();
    }

    Ptr GetParent() const override
    {
      return Ptr();
    }

  private:
    const Char Separator;
  };

  template<class It>
  Path::Ptr CreatePath(Char separator, It from, It to)
  {
    if (from != to)
    {
      return MakePtr<ParsedPath>(separator, from, to);
    }
    else
    {
      return MakePtr<EmptyPath>(separator);
    }
  }

  Path::Ptr CreatePath(Char separator, Strings::Array data)
  {
    if (!data.empty())
    {
      return MakePtr<ParsedPath>(separator, std::move(data));
    }
    else
    {
      return MakePtr<EmptyPath>(separator);
    }
  }
}  // namespace Analysis

namespace Analysis
{
  Path::Ptr ParsePath(const String& str, Char separator)
  {
    auto parsed = SplitPath(str, separator);
    return CreatePath(separator, std::move(parsed));
  }
}  // namespace Analysis
