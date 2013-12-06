/**
*
* @file
*
* @brief  Format syntax implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "grammar.h"
#include "syntax.h"
//common includes
#include <contract.h>
#include <iterator.h>
//std includes
#include <cctype>
#include <stack>

namespace
{
  inline uint_t ParseDecimalValue(const std::string& num)
  {
    Require(!num.empty());
    uint_t res = 0;
    for (RangeIterator<std::string::const_iterator> it(num.begin(), num.end()); it; ++it)
    {
      Require(0 != std::isdigit(*it));
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
        visitor.Match(tok.Value);
        return this;
      }
      else
      {
        Require(tok.Value.size() % 2 == 0);
        std::string val = tok.Value;
        while (!val.empty())
        {
          visitor.Match(val.substr(0, 2));
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

    virtual void MultipleTokensMatched(const std::string& /*lexeme*/, const LexicalAnalysis::TokenTypesSet& /*types*/)
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

    bool IsOperation() const
    {
      return Prec > 0;
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

  class RPNTranslation : public Binary::FormatTokensVisitor
  {
  public:
    RPNTranslation(Binary::FormatTokensVisitor& delegate)
      : Delegate(delegate)
      , LastIsMatch(false)
    {
    }

    virtual void Match(const std::string& val)
    {
      if (LastIsMatch)
      {
        FlushOperations();
      }
      Delegate.Match(val);
      LastIsMatch = true;
    }

    virtual void GroupStart()
    {
      FlushOperations();
      Ops.push(Operator(GROUP_START));
      Delegate.GroupStart();
      LastIsMatch = false;
    }

    virtual void GroupEnd()
    {
      FlushOperations();
      Require(!Ops.empty() && Ops.top().Value() == GROUP_START);
      Ops.pop();
      Delegate.GroupEnd();
      LastIsMatch = false;
    }

    virtual void Quantor(uint_t count)
    {
      FlushOperations();
      Delegate.Quantor(count);
      LastIsMatch = false;
    }

    virtual void Operation(const std::string& op)
    {
      const Operator newOp(op);
      FlushOperations(newOp);
      Ops.push(newOp);
      LastIsMatch = false;
    }

    void Flush()
    {
      while (!Ops.empty())
      {
        Require(Ops.top().IsOperation());
        FlushOperations();
      }
    }
  private:
    void FlushOperations()
    {
      while (!Ops.empty())
      {
        const Operator& topOp = Ops.top();
        if (!topOp.IsOperation())
        {
          break;
        }
        Delegate.Operation(topOp.Value());
        Ops.pop();
      }
    }
    void FlushOperations(const Operator& newOp)
    {
      while (!Ops.empty())
      {
        const Operator& topOp = Ops.top();
        if (!topOp.IsOperation())
        {
          break;
        }
        if ((newOp.LeftAssoc() && newOp.Precedence() <= topOp.Precedence())
          || (newOp.Precedence() < topOp.Precedence()))
        {
          Delegate.Operation(topOp.Value());
          Ops.pop();
        }
        else
        {
          break;
        }
      }
    }
  private:
    Binary::FormatTokensVisitor& Delegate;
    std::stack<Operator> Ops;
    bool LastIsMatch;
  };

  class SyntaxCheck : public Binary::FormatTokensVisitor
  {
  public:
    explicit SyntaxCheck(Binary::FormatTokensVisitor& delegate)
      : Delegate(delegate)
      , Position(0)
    {
    }

    virtual void Match(const std::string& val)
    {
      Delegate.Match(val);
      ++Position;
    }

    virtual void GroupStart()
    {
      GroupStarts.push(Position);
      Delegate.GroupStart();
    }

    virtual void GroupEnd()
    {
      Require(!GroupStarts.empty());
      Require(GroupStarts.top() != Position);
      Groups.push(Group(GroupStarts.top(), Position));
      GroupStarts.pop();
      Delegate.GroupEnd();
    }

    virtual void Quantor(uint_t count)
    {
      Require(Position != 0);
      Require(count != 0);
      Delegate.Quantor(count);
    }

    virtual void Operation(const std::string& op)
    {
      const std::size_t usedVals = Operator(op).Parameters();
      CheckAvailableParameters(usedVals, Position);
      Position = Position - usedVals + 1;
      Delegate.Operation(op);
    }
  private:
    void CheckAvailableParameters(std::size_t parameters, std::size_t position)
    {
      if (!parameters)
      {
        return;
      }
      const std::size_t start = GroupStarts.empty() ? 0 : GroupStarts.top();
      if (Groups.empty() || Groups.top().End < start)
      {
        Require(parameters + start <= position);
        return;
      }
      const Group top = Groups.top();
      const std::size_t nonGrouped = position - top.End;
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
    std::size_t Position;
    std::stack<std::size_t> GroupStarts;
    std::stack<Group> Groups;
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
