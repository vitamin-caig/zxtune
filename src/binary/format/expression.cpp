/**
*
* @file
*
* @brief  Binary expression compiler
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "expression.h"
#include "grammar.h"
#include "syntax.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <math/numeric.h>
//std includes
#include <cctype>
#include <functional>
#include <stack>
#include <vector>

namespace Binary
{
namespace FormatDSL
{
  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  class AnyValueToken : public Token
  {
  public:
    AnyValueToken()
    {
    }

    virtual bool Match(uint_t /*val*/) const
    {
      return true;
    }

    static Ptr Create()
    {
      static const AnyValueToken INSTANCE;
      return MakeSingletonPointer(INSTANCE);
    }
  };

  class MatchValueToken : public Token
  {
  public:
    explicit MatchValueToken(uint_t value)
      : Value(value)
    {
    }

    virtual bool Match(uint_t val) const
    {
      return val == Value;
    }

    static Ptr Create(PatternIterator& it)
    {
      Require(it && SYMBOL_TEXT == *it && ++it);
      const Char val = *it;
      ++it;
      return MakePtr<MatchValueToken>(val);
    }
  private:
    const uint_t Value;
  };

  class MatchMaskToken : public Token
  {
  public:
    MatchMaskToken(uint_t mask, uint_t value)
      : Mask(mask)
      , Value(value)
    {
      Require(Mask != 0 && Mask != 0xff);
    }

    virtual bool Match(uint_t val) const
    {
      return (Mask & val) == Value;
    }

    static Ptr Create(PatternIterator& it)
    {
      Require(it);
      uint_t mask = 0;
      uint_t value = 0;
      if (BINARY_MASK_TEXT == *it)
      {
        ++it;
        for (uint_t bitmask = 128; bitmask; bitmask >>= 1, ++it)
        {
          Require(it);
          Require(*it == ONE_BIT_TEXT || *it == ZERO_BIT_TEXT || *it == ANY_BIT_TEXT);
          switch (*it)
          {
          case ONE_BIT_TEXT:
            value |= bitmask;
          case ZERO_BIT_TEXT:
            mask |= bitmask;
            break;
          case ANY_BIT_TEXT:
            break;
          }
        }
      }
      else
      {
        const char hiNibble(*it);
        Require(++it);
        const char loNibble(*it);
        ++it;
        mask = NibbleToMask(hiNibble) * 16 + NibbleToMask(loNibble);
        value = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
      }
      switch (mask)
      {
      case 0:
        return AnyValueToken::Create();
      case 0xff:
        return MakePtr<MatchValueToken>(value);
      default:
        return MakePtr<MatchMaskToken>(mask, value);
      }
    }
  private:
    inline static uint_t NibbleToMask(char c)
    {
      Require(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c
        ? 0 : 0xf;
    }

    inline static uint_t NibbleToValue(char c)
    {
      Require(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c
        ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }
  private:
    const uint_t Mask;
    const uint_t Value;
  };

  inline uint_t ParseNumber(PatternIterator& it)
  {
    uint_t res = 0;
    while (it && std::isdigit(*it))
    {
      res = res * 10 + (*it - '0');
      ++it;
    }
    return res;
  }

  class MatchMultiplicityToken : public Token
  {
  public:
    MatchMultiplicityToken(uint_t mult)
      : Mult(mult)
    {
      Require(Mult > 0 && Mult < 128);
    }

    virtual bool Match(uint_t val) const
    {
      return 0 == (val % Mult);
    }

    static Ptr Create(PatternIterator& it)
    {
      Require(it && MULTIPLICITY_TEXT == *it && ++it);
      const uint_t val = ParseNumber(it);
      return MakePtr<MatchMultiplicityToken>(val);
    }
  private:
    const uint_t Mult;
  };

  uint_t GetSingleMatchedValue(const Token& tok)
  {
    const int_t NO_MATCHES = -1;
    int_t val = NO_MATCHES;
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (tok.Match(idx))
      {
        Require(NO_MATCHES == val);
        val = idx;
      }
    }
    Require(NO_MATCHES != val);
    return static_cast<uint_t>(val);
  }

  bool IsAnyByte(Token::Ptr tok)
  {
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (!tok->Match(idx))
      {
        return false;
      }
    }
    return true;
  }

  bool IsNoByte(Token::Ptr tok)
  {
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (tok->Match(idx))
      {
        return false;
      }
    }
    return true;
  }

  class MatchRangeToken : public Token
  {
  public:
    MatchRangeToken(uint_t from, uint_t to)
      : From(from)
      , To(to)
    {
    }

    virtual bool Match(uint_t val) const
    {
      return Math::InRange(val, From, To);
    }

    static Ptr Create(Ptr lh, Ptr rh)
    {
      const uint_t left = GetSingleMatchedValue(*lh);
      const uint_t right = GetSingleMatchedValue(*rh);
      Require(left < right);
      return MakePtr<MatchRangeToken>(left, right);
    }
  private:
    const uint_t From;
    const uint_t To;
  };

  struct Conjunction
  {
    static bool Execute(bool lh, bool rh)
    {
      return lh && rh;
    }
  };

  struct Disjunction
  {
    static bool Execute(bool lh, bool rh)
    {
      return lh || rh;
    }
  };

  template<class BinOp>
  class BinaryOperationToken : public Token
  {
  public:
    BinaryOperationToken(Ptr lh, Ptr rh)
      : Lh(lh)
      , Rh(rh)
    {
    }

    virtual bool Match(uint_t val) const
    {
      return BinOp::Execute(Lh->Match(val), Rh->Match(val));
    }

    static Ptr Create(Ptr lh, Ptr rh)
    {
      return MakePtr<BinaryOperationToken>(lh, rh);
    }
  private:
    const Ptr Lh;
    const Ptr Rh;
  };

  inline std::size_t ParseQuantor(PatternIterator& it)
  {
    Require(it && QUANTOR_BEGIN == *it);
    const std::size_t mult = ParseNumber(++it);
    Require(it && QUANTOR_END == *it);
    ++it;
    return mult;
  }

  Token::Ptr ParseSingleToken(const std::string& txt)
  {
    PatternIterator it(txt.begin(), txt.end());
    Require(it);
    switch (*it)
    {
    case ANY_BYTE_TEXT:
      ++it;
      return AnyValueToken::Create();
    case SYMBOL_TEXT:
      return MatchValueToken::Create(it);
    case MULTIPLICITY_TEXT:
      return MatchMultiplicityToken::Create(it);
    case BINARY_MASK_TEXT:
    default:
      return MatchMaskToken::Create(it);
    }
  }

  typedef std::vector<Token::Ptr> Pattern;

  Token::Ptr ParseOperation(const std::string& txt, Pattern& pat)
  {
    Require(!txt.empty());
    if (txt[0] == RANGE_TEXT ||
        txt[0] == CONJUNCTION_TEXT ||
        txt[0] == DISJUNCTION_TEXT)
    {
      Require(pat.size() >= 2);
      const Token::Ptr rh = pat.back();
      pat.pop_back();
      const Token::Ptr lh = pat.back();
      pat.pop_back();
      if (txt[0] == RANGE_TEXT)
      {
        const Token::Ptr res = MatchRangeToken::Create(lh, rh);
        Require(!IsAnyByte(res));
        return res;
      }
      else if (txt[0] == CONJUNCTION_TEXT)
      {
        return BinaryOperationToken<Conjunction>::Create(lh, rh);
      }
      else if (txt[0] == DISJUNCTION_TEXT)
      {
        const Token::Ptr res = BinaryOperationToken<Disjunction>::Create(lh, rh);
        Require(!IsAnyByte(res));
        return res;
      }
    }
    Require(false);
    return Token::Ptr();
  }

  class TokensFactory : public FormatTokensVisitor
  {
  public:
    virtual void Match(const std::string& val)
    {
      Result.push_back(ParseSingleToken(val));
      Require(!IsNoByte(Result.back()));
    }

    virtual void GroupStart()
    {
      GroupBegins.push(Result.size());
    }

    virtual void GroupEnd()
    {
      Require(!GroupBegins.empty());
      Groups.push(std::make_pair(GroupBegins.top(), Result.size()));
      GroupBegins.pop();
    }

    virtual void Quantor(uint_t count)
    {
      Require(count != 0);
      Require(!Result.empty());
      Pattern dup;
      if (!Groups.empty() && Groups.top().second == Result.size())
      {
        dup.assign(Result.begin() + Groups.top().first, Result.end());
        Groups.pop();
      }
      else
      {
        dup.push_back(Result.back());
      }
      for (std::size_t idx = 0; idx < count - 1; ++idx)
      {
        std::copy(dup.begin(), dup.end(), std::back_inserter(Result));
      }
    }

    virtual void Operation(const std::string& op)
    {
      Result.push_back(ParseOperation(op, Result));
      Require(!IsNoByte(Result.back()));
    }

    Pattern GetResult() const
    {
      Require(GroupBegins.empty());
      return Result;
    }
  private:
    Pattern Result;
    std::stack<std::size_t> GroupBegins;
    std::stack<std::pair<std::size_t, std::size_t> > Groups;
  };

  Pattern CompilePattern(const std::string& textPattern)
  {
    TokensFactory factory;
    const FormatTokensVisitor::Ptr check = CreatePostfixSynaxCheckAdapter(factory);
    ParseFormatNotationPostfix(textPattern, *check);
    return factory.GetResult();
  }

  class LinearExpression : public Expression
  {
  public:
    LinearExpression(std::size_t offset, Pattern::const_iterator from, Pattern::const_iterator to)
      : Offset(offset)
      , Pat(from, to)
    {
    }

    virtual std::size_t StartOffset() const
    {
      return Offset;
    }

    virtual ObjectIterator<Token::Ptr>::Ptr Tokens() const
    {
      return CreateRangedObjectIteratorAdapter(Pat.begin(), Pat.end());
    }
  private:
    const std::size_t Offset;
    const Pattern Pat;
  };
}
}

namespace Binary
{
namespace FormatDSL
{
  Expression::Ptr Expression::Parse(const std::string& notation)
  {
    const Pattern& pat = CompilePattern(notation);
    const Pattern::const_iterator first = pat.begin();
    const Pattern::const_iterator last = pat.end();
    const Pattern::const_iterator firstNotAny = std::find_if(first, last, std::not1(std::ptr_fun(&IsAnyByte)));
    Require(firstNotAny != last);
    const Pattern::const_iterator lastNotAny = std::find_if(pat.rbegin(), pat.rend(), std::not1(std::ptr_fun(&IsAnyByte))).base();
    const std::size_t offset = std::distance(first, firstNotAny);
    return MakePtr<LinearExpression>(offset, firstNotAny, lastNotAny);
  }
}
}
