/*
Abstract:
  Lexic analysis interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "lexic_analysis.h"
//common includes
#include <contract.h>
//std includes
#include <list>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>

namespace
{
  struct TokensSet
  {
    typedef boost::shared_ptr<const TokensSet> Ptr;

    std::string Lexeme;
    LexicalAnalysis::TokenTypesSet Types;

    void Add(const std::string& lexeme, LexicalAnalysis::TokenType type)
    {
      if (Empty())
      {
        Lexeme = lexeme;
      }
      else
      {
        Require(Lexeme == lexeme);
      }
      Types.Add(type);
    }

    bool Empty() const
    {
      return Types.Empty();
    }

    void Report(LexicalAnalysis::Grammar::Callback& cb) const
    {
      Require(!Empty());
      if (Types.Size() == 1)
      {
        cb.TokenMatched(Lexeme, Types.GetFirst());
      }
      else
      {
        cb.MultipleTokensMatched(Lexeme, Types);
      }
    }
  };
}

namespace LexicalAnalysis
{
  class ContextIndependentGrammar : public Grammar
  {
  public:
    virtual void AddTokenizer(Tokenizer::Ptr src)
    {
      Sources.push_back(src);
    }

    virtual void Analyse(const std::string& notation, Callback& cb) const
    {
      for (std::string::const_iterator cursor = notation.begin(), lim = notation.end(); cursor != lim; )
      {
        if (const TokensSet::Ptr tokens = ExtractLongestTokens(cursor, lim))
        {
          tokens->Report(cb);
          cursor += tokens->Lexeme.size();
        }
        else
        {
          cb.AnalysisError(notation, cursor - notation.begin());
          break;
        }
      }
    }
  private:
    TokensSet::Ptr ExtractLongestTokens(std::string::const_iterator lexemeStart, std::string::const_iterator lim) const
    {
      std::list<boost::shared_ptr<TokensSet> > context;
      std::list<Tokenizer::Ptr> candidates = Sources;
      for (std::string::const_iterator lexemeEnd = lexemeStart + 1; !candidates.empty(); ++lexemeEnd)
      {
        const std::string lexeme(lexemeStart, lexemeEnd);
        context.push_back(boost::make_shared<TokensSet>());
        TokensSet& target = *context.back();
        for (std::list<Tokenizer::Ptr>::iterator tokIt = candidates.begin(), tokLim = candidates.end(); tokIt != tokLim;)
        {
          const Tokenizer::Ptr tokenizer = *tokIt;
          switch (const TokenType result = tokenizer->Parse(lexeme))
          {
          case INVALID_TOKEN:
            tokIt = candidates.erase(tokIt);
            break;
          case INCOMPLETE_TOKEN:
            ++tokIt;
            break;
          default:
            target.Add(lexeme, result);
            ++tokIt;
            break;
          }
        }
        if (lexemeEnd == lim)
        {
          break;
        }
      }
      context.remove_if(boost::mem_fn(&TokensSet::Empty));
      if (context.empty())
      {
        return TokensSet::Ptr();
      }
      return context.back();
    }
  private:
    std::list<Tokenizer::Ptr> Sources;
  };

  Grammar::RWPtr CreateContextIndependentGrammar()
  {
    return boost::make_shared<ContextIndependentGrammar>();
  }
}
