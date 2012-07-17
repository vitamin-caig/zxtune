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
//std includes
#include <stack>

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

    virtual void MultipleTokensMatched(const std::string& /*lexeme*/, const std::set<LexicalAnalysis::TokenType>& /*types*/)
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

  const std::string GROUP_START(1, Binary::GROUP_BEGIN);

  struct Operator
  {
  public:
    Operator()
      : Val()
      , Prec(0)
    {
    }

    explicit Operator(const std::string& op)
      : Val(op)
      , Prec(0)
    {
      Require(!Val.empty());
      switch (Val[0])
      {
      case Binary::RANGE_TEXT:
        ++Prec;
      case Binary::CONJUNCTION_TEXT:
        ++Prec;
      case Binary::DISJUNCTION_TEXT:
        ++Prec;
      }
    }

    std::string Value() const
    {
      return Val;
    }

    std::size_t Precedence() const
    {
      return Prec;
    }

    std::size_t Parameters() const
    {
      return 2;
    }

    bool LeftAssoc() const
    {
      return true;
    }
  private:
    const std::string Val;
    std::size_t Prec;
  };

  class SyntaxCheck : public Binary::FormatTokensVisitor
  {
  public:
    explicit SyntaxCheck(Binary::FormatTokensVisitor& delegate)
      : Delegate(delegate)
      , ValuesInOutput(0)
    {
    }

    virtual void Value(const std::string& val)
    {
      Delegate.Value(val);
      ++ValuesInOutput;
    }

    virtual void GroupStart()
    {
      GroupStarts.push(ValuesInOutput);
      Delegate.GroupStart();
    }

    virtual void GroupEnd()
    {
      Require(!GroupStarts.empty());
      Require(GroupStarts.top() != ValuesInOutput);
      Groups.push(Group(GroupStarts.top(), ValuesInOutput));
      GroupStarts.pop();
      Delegate.GroupEnd();
    }

    virtual void Quantor(uint_t count)
    {
      Require(ValuesInOutput != 0);
      Require(count != 0);
      Delegate.Quantor(count);
    }

    virtual void Operation(const std::string& op)
    {
      const std::size_t usedVals = Operator(op).Parameters();
      CheckAvailableParameters(usedVals, ValuesInOutput);
      ValuesInOutput = ValuesInOutput - usedVals + 1;
      Delegate.Operation(op);
    }
  private:
    void CheckAvailableParameters(std::size_t parameters, std::size_t valuesAvail)
    {
      if (!parameters)
      {
        return;
      }
      const std::size_t start = GroupStarts.empty() ? 0 : GroupStarts.top();
      if (Groups.empty() || Groups.top().End < start)
      {
        Require(parameters + start <= valuesAvail);
        return;
      }
      const Group top = Groups.top();
      const std::size_t nonGrouped = valuesAvail - top.End;
      if (nonGrouped < parameters)
      {
        if (nonGrouped)
        {
          CheckAvailableParameters(parameters - nonGrouped, top.End);
        }
        else
        {
          Require(top.Size() == 1);
          Groups.pop();
          CheckAvailableParameters(parameters - 1, top.Begin);
          Groups.push(top);
        }
      }
    }
  private:
    struct Group
    {
      Group(std::size_t begin, std::size_t end)
        : Begin(begin)
        , End(end)
      {
      }

      Group()
        : Begin()
        , End()
      {
      }

      std::size_t Size() const
      {
        return End - Begin;
      }

      std::size_t Begin;
      std::size_t End;
    };
  private:
    Binary::FormatTokensVisitor& Delegate;
    std::size_t ValuesInOutput;
    std::stack<std::size_t> GroupStarts;
    std::stack<Group> Groups;
  };

  class RPNTranslation : public Binary::FormatTokensVisitor
  {
  public:
    RPNTranslation(Binary::FormatTokensVisitor& delegate)
      : Delegate(delegate)
      , LastIsValue(false)
    {
    }

    virtual void Value(const std::string& val)
    {
      if (LastIsValue)
      {
        while (!Ops.empty())
        {
          const Operator& topOp = Ops.top();
          if (topOp.Value() == GROUP_START)
          {
            break;
          }
          Delegate.Operation(topOp.Value());
          Ops.pop();
        }
      }
      Delegate.Value(val);
      LastIsValue = true;
    }

    virtual void GroupStart()
    {
      Ops.push(Operator(GROUP_START));
      Delegate.GroupStart();
      LastIsValue = false;
    }

    virtual void GroupEnd()
    {
      for (;;)
      {
        Require(!Ops.empty());
        const Operator& topOp = Ops.top();
        if (topOp.Value() == GROUP_START)
        {
          Ops.pop();
          break;
        }
        Delegate.Operation(topOp.Value());
        Ops.pop();
      }
      Delegate.GroupEnd();
      LastIsValue = false;
    }

    virtual void Quantor(uint_t count)
    {
      Delegate.Quantor(count);
      LastIsValue = false;
    }

    virtual void Operation(const std::string& op)
    {
      const Operator newOp(op);
      while (!Ops.empty())
      {
        const Operator& prevOp = Ops.top();
        if ((newOp.LeftAssoc() && newOp.Precedence() <= prevOp.Precedence())
          || (newOp.Precedence() < prevOp.Precedence()))
        {
          Delegate.Operation(prevOp.Value());
          Ops.pop();
        }
        else
        {
          break;
        }
      }
      Ops.push(newOp);
      LastIsValue = false;
    }

    void Flush()
    {
      while (!Ops.empty())
      {
        const std::string val = Ops.top().Value();
        Require(val != GROUP_START);
        Delegate.Operation(val);
        Ops.pop();
      }
    }
  private:
    Binary::FormatTokensVisitor& Delegate;
    std::stack<Operator> Ops;
    bool LastIsValue;
  };
}

namespace Binary
{
  void ParseFormatNotation(const std::string& notation, FormatTokensVisitor& visitor)
  {
    ParseFSM fsm(visitor);
    CreateFormatGrammar()->Analyse(notation, fsm);
  }

  void ParseFormatNotationPostfix(const std::string& notation, FormatTokensVisitor& visitor)
  {
    RPNTranslation rpn(visitor);
    ParseFormatNotation(notation, rpn);
    rpn.Flush();
  }

  FormatTokensVisitor::Ptr CreatePostfixSynaxCheckAdapter(FormatTokensVisitor& visitor)
  {
    return FormatTokensVisitor::Ptr(new SyntaxCheck(visitor));
  }
}
