/**
 *
 * @file
 *
 * @brief  Parameters identifiers set
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/identifier.h"

#include "string_type.h"
#include "string_view.h"

#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

namespace Parameters
{
  //! @brief Non-owning set of parameter identifiers
  //! @details Views point either into static literals or into caller-owned serialized storage,
  //! which must outlive the set
  class IdentifiersSet
  {
  public:
    IdentifiersSet() = default;

    IdentifiersSet(std::initializer_list<Identifier> items)
    {
      for (const auto& item : items)
      {
        Add(item);
      }
    }

    //! @brief Parse newline-delimited identifiers; the result views into str, which must outlive it
    static IdentifiersSet Parse(StringView str)
    {
      std::vector<StringView> items;
      while (!str.empty())
      {
        const auto delim = str.find('\n');
        const auto part = StringView::npos == delim ? str : str.substr(0, delim);
        if (!part.empty())
        {
          items.emplace_back(part);
        }
        if (StringView::npos == delim)
        {
          break;
        }
        str = str.substr(delim + 1);
      }
      return IdentifiersSet{std::move(items)};
    }

    //! @brief Rejected: the set would outlive the temporary storage
    template<class T>
    requires std::is_same_v<T, String> static IdentifiersSet Parse(T&&) = delete;

    //! @brief Newline-delimited representation
    String ToString() const
    {
      String result;
      for (const auto& item : Items)
      {
        if (!result.empty())
        {
          result += '\n';
        }
        result += item;
      }
      return result;
    }

    bool Contains(Identifier name) const
    {
      const StringView view = name;
      return Items.cend() != std::find(Items.cbegin(), Items.cend(), view);
    }

  private:
    friend IdentifiersSet operator|(const IdentifiersSet& lh, const IdentifiersSet& rh);
    friend IdentifiersSet operator|(const IdentifiersSet& lh, Identifier rh);

    //! @brief Build from ready views; the caller keeps the referenced storage alive
    explicit IdentifiersSet(std::vector<StringView> items)
    {
      for (const auto& item : items)
      {
        Add(item);
      }
    }

    void Add(Identifier name)
    {
      if (!Contains(name))
      {
        Items.emplace_back(name);
      }
    }

  private:
    std::vector<StringView> Items;
  };

  inline IdentifiersSet operator|(const IdentifiersSet& lh, const IdentifiersSet& rh)
  {
    auto items = lh.Items;
    items.insert(items.end(), rh.Items.begin(), rh.Items.end());
    return IdentifiersSet{std::move(items)};
  }

  inline IdentifiersSet operator|(const IdentifiersSet& lh, Identifier rh)
  {
    auto items = lh.Items;
    items.emplace_back(rh);
    return IdentifiersSet{std::move(items)};
  }
}  // namespace Parameters
