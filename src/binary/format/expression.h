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
    typedef std::shared_ptr<const Predicate> Ptr;
    virtual ~Predicate() = default;

    virtual bool Match(uint_t val) const = 0;
  };

  typedef std::vector<Predicate::Ptr> Pattern;

  class Expression
  {
  public:
    typedef std::unique_ptr<const Expression> Ptr;
    virtual ~Expression() = default;

    virtual std::size_t StartOffset() const = 0;
    virtual const Pattern& Predicates() const = 0;

    static Ptr Parse(StringView notation);
  };
}  // namespace Binary::FormatDSL
