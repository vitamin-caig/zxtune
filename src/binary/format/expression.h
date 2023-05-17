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
#include <types.h>
// std includes
#include <list>
#include <memory>
#include <vector>

namespace Binary::FormatDSL
{
  class Predicate
  {
  public:
    using Ptr = std::shared_ptr<const Predicate>;
    virtual ~Predicate() = default;

    virtual bool Match(uint_t val) const = 0;
  };

  using Pattern = std::vector<Predicate::Ptr>;

  class Expression
  {
  public:
    using Ptr = std::unique_ptr<const Expression>;
    virtual ~Expression() = default;

    virtual std::size_t StartOffset() const = 0;
    virtual const Pattern& Predicates() const = 0;

    static Ptr Parse(StringView notation);
  };
}  // namespace Binary::FormatDSL
