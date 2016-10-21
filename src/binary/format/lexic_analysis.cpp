/**
*
* @file
*
* @brief  Lexic analyser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "lexic_analysis.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//std includes
#include <functional>
#include <list>

namespace LexicalAnalysis
{
  struct TokensSet
  {
    typedef std::shared_ptr<const TokensSet> Ptr;
    typedef std::shared_ptr<TokensSet> RWPtr;

    std::string Lexeme;
    TokenTypesSet Types;

    void Add(const std::string& lexeme, TokenType type)
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

    void Report(Grammar::Callback& cb) const
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

  class ContextIndependentGrammar : public Grammar
  {
  public:
    void AddTokenizer(Tokenizer::Ptr src) override
    {
      Sources.push_back(src);
    }

    void Analyse(const std::string& notation, Callback& cb) const override
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
      std::list<TokensSet::RWPtr> context;
      std::list<Tokenizer::Ptr> candidates = Sources;
      for (std::string::const_iterator lexemeEnd = lexemeStart + 1; !candidates.empty(); ++lexemeEnd)
      {
        const std::string lexeme(lexemeStart, lexemeEnd);
        context.push_back(MakeRWPtr<TokensSet>());
        TokensSet& target = *context.back();
        for (auto tokIt = candidates.begin(), tokLim = candidates.end(); tokIt != tokLim;)
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
      context.remove_if(std::mem_fn(&TokensSet::Empty));
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
    return MakeRWPtr<ContextIndependentGrammar>();
  }
}
