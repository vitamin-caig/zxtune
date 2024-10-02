/**
 *
 * @file
 *
 * @brief  Binary format expression interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// std includes
#include <memory>
#include <span>

namespace Binary::FormatDSL
{
  class Predicate
  {
  public:
    virtual ~Predicate() = default;

    virtual bool Match(uint_t val) const = 0;
  };

  class Expression
  {
  public:
    using Ptr = std::unique_ptr<const Expression>;
    virtual ~Expression() = default;

    virtual std::size_t StartOffset() const = 0;
    virtual std::span<const Predicate* const> Predicates() const = 0;

    static Ptr Parse(StringView notation);
  };
}  // namespace Binary::FormatDSL
