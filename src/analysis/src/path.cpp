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
#include <string_view.h>
// library includes
#include <analysis/path.h>
#include <strings/array.h>
#include <strings/join.h>
#include <strings/split.h>
// std includes
#include <functional>

namespace Analysis
{
  std::vector<StringView> SplitPath(StringView str, char separator)
  {
    auto parts = Strings::Split(str, separator);
    const auto newEnd = std::remove_if(parts.begin(), parts.end(), std::mem_fn(&StringView::empty));
    parts.erase(newEnd, parts.end());
    return parts;
  }

  String JoinPath(const Strings::Array& arr, char separator)
  {
    return Strings::Join(arr, StringView(&separator, 1));
  }

  template<class It>
  Path::Ptr CreatePath(char separator, It from, It to);

  Path::Ptr CreatePath(char separator, Strings::Array data);

  class ParsedPath : public Path
  {
  public:
    template<class Iter>
    ParsedPath(char separator, Iter from, Iter to)
      : Components(from, to)
      , Separator(separator)
    {
      Require(from != to);
    }

    ParsedPath(char separator, Strings::Array data)
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

    std::span<const String> Elements() const override
    {
      return {Components};
    }

    Ptr Append(StringView element) const override
    {
      const auto& newOne = SplitPath(element, Separator);
      Strings::Array result(Components.size() + newOne.size());
      std::copy(newOne.begin(), newOne.end(), std::copy(Components.begin(), Components.end(), result.begin()));
      return CreatePath(Separator, std::move(result));
    }

    Ptr Extract(StringView startPath) const override
    {
      const auto& subSplitted = SplitPath(startPath, Separator);
      if (subSplitted.size() > Components.size())
      {
        return {};
      }
      const auto iters = std::mismatch(subSplitted.begin(), subSplitted.end(), Components.begin());
      if (iters.first != subSplitted.end())
      {
        return {};
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
    const char Separator;
  };

  class EmptyPath : public Path
  {
  public:
    explicit EmptyPath(char separator)
      : Separator(separator)
    {}

    bool Empty() const override
    {
      return true;
    }

    String AsString() const override
    {
      return {};
    }

    std::span<const String> Elements() const override
    {
      return {};
    }

    Ptr Append(StringView element) const override
    {
      return ParsePath(element, Separator);
    }

    Ptr Extract(StringView startPath) const override
    {
      return startPath.empty() ? MakePtr<EmptyPath>(Separator) : Ptr();
    }

    Ptr GetParent() const override
    {
      return {};
    }

  private:
    const char Separator;
  };

  template<class It>
  Path::Ptr CreatePath(char separator, It from, It to)
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

  Path::Ptr CreatePath(char separator, Strings::Array data)
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
  Path::Ptr ParsePath(StringView str, char separator)
  {
    const auto& parsed = SplitPath(str, separator);
    return CreatePath(separator, parsed.begin(), parsed.end());
  }
}  // namespace Analysis
