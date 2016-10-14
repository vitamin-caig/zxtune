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

//common includes
#include <iterator.h>
#include <types.h>
//std includes
#include <memory>
#include <string>

namespace Binary
{
  namespace FormatDSL
  {
    class Token
    {
    public:
      typedef std::shared_ptr<const Token> Ptr;
      virtual ~Token() = default;

      virtual bool Match(uint_t val) const = 0;
    };

    typedef std::vector<Token::Ptr> Pattern;

    class Expression
    {
    public:
      typedef std::unique_ptr<const Expression> Ptr;
      virtual ~Expression() = default;

      virtual std::size_t StartOffset() const = 0;
      virtual ObjectIterator<Token::Ptr>::Ptr Tokens() const = 0;

      static Ptr Parse(const std::string& notation);
    };
  }
}
