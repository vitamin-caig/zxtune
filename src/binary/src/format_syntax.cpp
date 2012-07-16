/*
Abstract:
  Format syntax implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "format_grammar.h"
#include "format_syntax.h"
//common includes
#include <contract.h>
#include <iterator.h>

namespace
{
  inline uint_t ParseDecimalValue(const std::string& num)
  {
    Require(!num.empty());
    uint_t res = 0;
    for (RangeIterator<std::string::const_iterator> it(num.begin(), num.end()); it; ++it)
    {
      Require(std::isdigit(*it));
      res = res * 10 + (*it - '0');
    }
    return res;
  }

  struct Token
  {
    LexicalAnalysis::TokenType Type;
    std::string Value;

    Token()
      : Type(Binary::DELIMITER)
      , Value(" ")
    {
    }

    Token(LexicalAnalysis::TokenType type, const std::string& lexeme)
      : Type(type)
      , Value(lexeme)
    {
    }
  };

  class State
  {
  public:
    virtual ~State() {}

    virtual const State* Transition(const Token& tok, Binary::FormatTokensVisitor& visitor) const = 0;

    static const State* Initial();
    static const State* Quantor();
    static const State* QuantorEnd();
    static const State* Error();
  };

  class InitialStateType : public State
  {
  public:
    virtual const State* Transition(const Token& tok, Binary::FormatTokensVisitor& visitor) const
    {
      switch (tok.Type)
      {
      case Binary::DELIMITER:
        return this;
      case Binary::OPERATION:
        return ParseOperation(tok, visitor);
      case Binary::CONSTANT:
      case Binary::MASK:
        return ParseValue(tok, visitor);
      default:
        return State::Error();
      }
    }
  private:
    const State* ParseOperation(const Token& tok, Binary::FormatTokensVisitor& visitor) const
    {
      Require(tok.Value.size() == 1);
      switch (tok.Value[0])
      {
      case Binary::GROUP_BEGIN:
        visitor.GroupStart();
        return this;
      case Binary::GROUP_END:
        visitor.GroupEnd();
        return this;
      case Binary::QUANTOR_BEGIN:
        return State::Quantor();
      case Binary::RANGE_TEXT:
      case Binary::CONJUNCTION_TEXT:
      case Binary::DISJUNCTION_TEXT:
        visitor.Operation(tok.Value);
        return this;
      default:
        return State::Error();
      }
    }

    const State* ParseValue(const Token& tok, Binary::FormatTokensVisitor& visitor) const
    {
      if (tok.Value[0] == Binary::BINARY_MASK_TEXT ||
          tok.Value[0] == Binary::MULTIPLICITY_TEXT ||
          tok.Value[0] == Binary::ANY_BYTE_TEXT ||
          tok.Value[0] == Binary::SYMBOL_TEXT)
      {
        visitor.Value(tok.Value);
        return this;
      }
      else
      {
        Require(tok.Value.size() % 2 == 0);
        std::string val = tok.Value;
        while (!val.empty())
        {
          visitor.Value(val.substr(0, 2));
          val = val.substr(2);
        }
        return this;
      }
    }
  };

  class QuantorStateType : public State
  {
  public:
    virtual const State* Transition(const Token& tok, Binary::FormatTokensVisitor& visitor) const
    {
      if (tok.Type == Binary::CONSTANT)
      {
        const uint_t num = ParseDecimalValue(tok.Value);
        visitor.Quantor(num);
        return State::QuantorEnd();
      }
      else
      {
        return State::Error();
      }
    }
  };

  class QuantorEndType : public State
  {
  public:
    virtual const State* Transition(const Token& tok, Binary::FormatTokensVisitor& /*visitor*/) const
    {
      Require(tok.Type == Binary::OPERATION);
      Require(tok.Value == std::string(1, Binary::QUANTOR_END));
      return State::Initial();
    }
  };

  class ErrorStateType : public State
  {
  public:
    virtual const State* Transition(const Token& /*token*/, Binary::FormatTokensVisitor& /*visitor*/) const
    {
      return this;
    }
  };

  const State* State::Initial()
  {
    static const InitialStateType instance;
    return &instance;
  }

  const State* State::Quantor()
  {
    static const QuantorStateType instance;
    return &instance;
  }

  const State* State::QuantorEnd()
  {
    static const QuantorEndType instance;
    return &instance;
  }

  const State* State::Error()
  {
    static const ErrorStateType instance;
    return &instance;
  }
}

namespace
{
  class ParseFSM : public LexicalAnalysis::Grammar::Callback
  {
  public:
    explicit ParseFSM(Binary::FormatTokensVisitor& visitor)
      : CurState(State::Initial())
      , Visitor(visitor)
    {
    }

    virtual void TokenMatched(const std::string& lexeme, LexicalAnalysis::TokenType type)
    {
      CurState = CurState->Transition(Token(type, lexeme), Visitor);
      Require(CurState != State::Error());
    }

    virtual void MultipleTokensMatched(const std::string& /*lexeme*/, const std::set<LexicalAnalysis::TokenType>& types)
    {
      Require(false);
    }

    virtual void AnalysisError(const std::string& /*notation*/, std::size_t /*position*/)
    {
      Require(false);
    }
  private:
    const State* CurState;
    Binary::FormatTokensVisitor& Visitor;
  };
}

namespace Binary
{
  void ParseFormatNotation(const std::string& notation, FormatTokensVisitor& visitor)
  {
    ParseFSM fsm(visitor);
    CreateFormatGrammar()->Analyse(notation, fsm);
  }
}
