/*
Abstract:
  Lexic analysis interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BINARY_LEXIC_ANALYSIS
#define BINARY_LEXIC_ANALYSIS

//common includes
#include <types.h>
//library includes
#include <math/bitops.h>
//std includes
#include <string>
//boost includes
#include <boost/shared_ptr.hpp>

namespace LexicalAnalysis
{
  typedef uint_t TokenType;

  const TokenType INVALID_TOKEN = TokenType(-1);
  const TokenType INCOMPLETE_TOKEN = TokenType(-2);
  
  class TokenTypesSet
  {
  public:
    TokenTypesSet()
      : Mask()
    {
    }
    
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
    uint_t Mask;
  };

  class Tokenizer
  {
  public:
    typedef boost::shared_ptr<const Tokenizer> Ptr;
    virtual ~Tokenizer() {}

    virtual TokenType Parse(const std::string& lexeme) const = 0;
  };

  class Grammar
  {
  public:
    typedef boost::shared_ptr<const Grammar> Ptr;
    typedef boost::shared_ptr<Grammar> RWPtr;
    virtual ~Grammar() {}

    virtual void AddTokenizer(Tokenizer::Ptr src) = 0;
    
    class Callback
    {
    public:
      virtual ~Callback() {}

      virtual void TokenMatched(const std::string& lexeme, TokenType type) = 0;
      virtual void MultipleTokensMatched(const std::string& lexeme, const TokenTypesSet& types) = 0;
      virtual void AnalysisError(const std::string& notation, std::size_t position) = 0;
    };
    virtual void Analyse(const std::string& notation, Callback& cb) const = 0;
  };

  Grammar::RWPtr CreateContextIndependentGrammar();
}

#endif //BINARY_LEXIC_ANALYSIS
