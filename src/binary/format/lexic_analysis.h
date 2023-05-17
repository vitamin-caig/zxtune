/**
 *
 * @file
 *
 * @brief  Lexic analyser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <math/bitops.h>
// std includes
#include <cassert>
#include <memory>

namespace LexicalAnalysis
{
  using TokenType = uint_t;

  const TokenType INVALID_TOKEN = TokenType(-1);
  const TokenType INCOMPLETE_TOKEN = TokenType(-2);

  class TokenTypesSet
  {
  public:
    TokenTypesSet() = default;

    void Add(TokenType type)
    {
      assert(type < 8 * sizeof(Mask));
      Mask |= 1 << type;
    }

    bool Empty() const
    {
      return Mask == 0;
    }

    std::size_t Size() const
    {
      return Math::CountBits(Mask);
    }

    TokenType GetFirst() const
    {
      assert(!Empty());
      TokenType res = 0;
      while (!Contains(res))
      {
        ++res;
      }
      return res;
    }

    bool Contains(TokenType type) const
    {
      return 0 != (Mask & (1 << type));
    }

  private:
    uint_t Mask = 0;
  };

  class Tokenizer
  {
  public:
    using Ptr = std::unique_ptr<const Tokenizer>;
    virtual ~Tokenizer() = default;

    virtual TokenType Parse(StringView lexeme) const = 0;
  };

  class Grammar
  {
  public:
    using Ptr = std::shared_ptr<const Grammar>;
    using RWPtr = std::shared_ptr<Grammar>;
    virtual ~Grammar() = default;

    virtual void AddTokenizer(Tokenizer::Ptr src) = 0;

    class Callback
    {
    public:
      virtual ~Callback() = default;

      virtual void TokenMatched(StringView lexeme, TokenType type) = 0;
      virtual void MultipleTokensMatched(StringView lexeme, const TokenTypesSet& types) = 0;
      virtual void AnalysisError(StringView notation, std::size_t position) = 0;
    };
    virtual void Analyse(StringView notation, Callback& cb) const = 0;
  };

  Grammar::RWPtr CreateContextIndependentGrammar();
}  // namespace LexicalAnalysis
