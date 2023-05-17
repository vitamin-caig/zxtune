/**
 *
 * @file
 *
 * @brief  Binary expression compiler
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "binary/format/expression.h"
#include "binary/format/grammar.h"
#include "binary/format/syntax.h"
// common includes
#include <contract.h>
#include <iterator.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <math/numeric.h>
// std includes
#include <cctype>
#include <functional>
#include <stack>
#include <vector>

namespace Binary::FormatDSL
{
  using PatternIterator = RangeIterator<StringView::const_iterator>;

  class AnyValuePredicate : public Predicate
  {
  public:
    AnyValuePredicate() = default;

    bool Match(uint_t /*val*/) const override
    {
      return true;
    }

    static Ptr Create()
    {
      static const AnyValuePredicate INSTANCE;
      return MakeSingletonPointer(INSTANCE);
    }
  };

  class MatchValuePredicate : public Predicate
  {
  public:
    explicit MatchValuePredicate(uint_t value)
      : Value(value)
    {}

    bool Match(uint_t val) const override
    {
      return val == Value;
    }

    static Ptr Create(PatternIterator& it)
    {
      Require(it && SYMBOL_TEXT == *it && ++it);
      const char val = *it;
      ++it;
      return MakePtr<MatchValuePredicate>(val);
    }

  private:
    const uint_t Value;
  };

  class MatchMaskPredicate : public Predicate
  {
  public:
    MatchMaskPredicate(uint_t mask, uint_t value)
      : Mask(mask)
      , Value(value)
    {
      Require(Mask != 0 && Mask != 0xff);
    }

    bool Match(uint_t val) const override
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
            [[fallthrough]];
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
        return AnyValuePredicate::Create();
      case 0xff:
        return MakePtr<MatchValuePredicate>(value);
      default:
        return MakePtr<MatchMaskPredicate>(mask, value);
      }
    }

  private:
    inline static uint_t NibbleToMask(char c)
    {
      Require(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c ? 0 : 0xf;
    }

    inline static uint_t NibbleToValue(char c)
    {
      Require(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
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

  class MatchMultiplicityPredicate : public Predicate
  {
  public:
    explicit MatchMultiplicityPredicate(uint_t mult)
      : Mult(mult)
    {
      Require(Mult < 128);
    }

    bool Match(uint_t val) const override
    {
      return 0 == (val % Mult);
    }

    static Ptr Create(PatternIterator& it)
    {
      Require(it && MULTIPLICITY_TEXT == *it && ++it);
      const uint_t val = ParseNumber(it);
      return MakePtr<MatchMultiplicityPredicate>(val);
    }

  private:
    const uint_t Mult;
  };

  uint_t GetSingleMatchedValue(const Predicate& pred)
  {
    const int_t NO_MATCHES = -1;
    int_t val = NO_MATCHES;
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (pred.Match(idx))
      {
        Require(NO_MATCHES == val);
        val = idx;
      }
    }
    Require(NO_MATCHES != val);
    return static_cast<uint_t>(val);
  }

  bool IsAnyByte(const Predicate& pred)
  {
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (!pred.Match(idx))
      {
        return false;
      }
    }
    return true;
  }

  bool IsNoByte(const Predicate& pred)
  {
    for (uint_t idx = 0; idx != 256; ++idx)
    {
      if (pred.Match(idx))
      {
        return false;
      }
    }
    return true;
  }

  bool NotAnyByte(const Predicate::Ptr& p)
  {
    return !IsAnyByte(*p);
  }

  class MatchRangePredicate : public Predicate
  {
  public:
    MatchRangePredicate(uint_t from, uint_t to)
      : From(from)
      , To(to)
    {}

    bool Match(uint_t val) const override
    {
      return Math::InRange(val, From, To);
    }

    static Ptr Create(const Predicate& lh, const Predicate& rh)
    {
      const uint_t left = GetSingleMatchedValue(lh);
      const uint_t right = GetSingleMatchedValue(rh);
      Require(left < right);
      return MakePtr<MatchRangePredicate>(left, right);
    }

  private:
    const uint_t From;
    const uint_t To;
  };

  struct Conjunction
  {
    static bool Execute(uint_t val, const Predicate& lh, const Predicate& rh)
    {
      return lh.Match(val) && rh.Match(val);
    }
  };

  struct Disjunction
  {
    static bool Execute(uint_t val, const Predicate& lh, const Predicate& rh)
    {
      return lh.Match(val) || rh.Match(val);
    }
  };

  template<class BinOp>
  class BinaryOperationPredicate : public Predicate
  {
  public:
    BinaryOperationPredicate(Ptr lh, Ptr rh)
      : Lh(std::move(lh))
      , Rh(std::move(rh))
    {}

    bool Match(uint_t val) const override
    {
      return BinOp::Execute(val, *Lh, *Rh);
    }

    static Ptr Create(Ptr lh, Ptr rh)
    {
      return MakePtr<BinaryOperationPredicate>(std::move(lh), std::move(rh));
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

  Predicate::Ptr ParseSinglePredicate(StringView txt)
  {
    PatternIterator it(txt.begin(), txt.end());
    Require(it);
    switch (*it)
    {
    case ANY_BYTE_TEXT:
      ++it;
      return AnyValuePredicate::Create();
    case SYMBOL_TEXT:
      return MatchValuePredicate::Create(it);
    case MULTIPLICITY_TEXT:
      return MatchMultiplicityPredicate::Create(it);
    case BINARY_MASK_TEXT:
    default:
      return MatchMaskPredicate::Create(it);
    }
  }

  Predicate::Ptr ParseOperation(StringView txt, Pattern& pat)
  {
    Require(!txt.empty());
    if (txt[0] == RANGE_TEXT || txt[0] == CONJUNCTION_TEXT || txt[0] == DISJUNCTION_TEXT)
    {
      Require(pat.size() >= 2);
      auto rh = std::move(pat.back());
      pat.pop_back();
      auto lh = std::move(pat.back());
      pat.pop_back();
      if (txt[0] == RANGE_TEXT)
      {
        return MatchRangePredicate::Create(*lh, *rh);
      }
      else if (txt[0] == CONJUNCTION_TEXT)
      {
        return BinaryOperationPredicate<Conjunction>::Create(std::move(lh), std::move(rh));
      }
      else if (txt[0] == DISJUNCTION_TEXT)
      {
        return BinaryOperationPredicate<Disjunction>::Create(std::move(lh), std::move(rh));
      }
    }
    Require(false);
    return Predicate::Ptr();
  }

  class PredicatesFactory : public FormatTokensVisitor
  {
  public:
    void Match(StringView val) override
    {
      Result.push_back(ParseSinglePredicate(val));
      Require(!IsNoByte(*Result.back()));
    }

    void GroupStart() override
    {
      GroupBegins.push(Result.size());
    }

    void GroupEnd() override
    {
      Require(!GroupBegins.empty());
      Groups.push(std::make_pair(GroupBegins.top(), Result.size()));
      GroupBegins.pop();
    }

    void Quantor(uint_t count) override
    {
      Require(count != 0);
      Require(!Result.empty());
      Pattern dup;
      if (!Groups.empty() && Groups.top().second == Result.size())
      {
        auto start = Result.begin();
        std::advance(start, Groups.top().first);
        dup.assign(start, Result.end());
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

    void Operation(StringView op) override
    {
      Result.emplace_back(ParseOperation(op, Result));
      Require(!IsAnyByte(*Result.back()));
      Require(!IsNoByte(*Result.back()));
    }

    Pattern CaptureResult()
    {
      Require(GroupBegins.empty());
      return std::move(Result);
    }

  private:
    Pattern Result;
    std::stack<std::size_t> GroupBegins;
    std::stack<std::pair<std::size_t, std::size_t> > Groups;
  };

  Pattern CompilePattern(StringView textPattern)
  {
    PredicatesFactory factory;
    const FormatTokensVisitor::Ptr check = CreatePostfixSyntaxCheckAdapter(factory);
    ParseFormatNotationPostfix(textPattern, *check);
    return factory.CaptureResult();
  }

  class LinearExpression : public Expression
  {
  public:
    LinearExpression(std::size_t offset, Pattern pat)
      : Offset(offset)
      , Pat(std::move(pat))
    {}

    std::size_t StartOffset() const override
    {
      return Offset;
    }

    const Pattern& Predicates() const override
    {
      return Pat;
    }

  private:
    const std::size_t Offset;
    const Pattern Pat;
  };
}  // namespace Binary::FormatDSL

namespace Binary::FormatDSL
{
  Expression::Ptr Expression::Parse(StringView notation)
  {
    auto pat = CompilePattern(notation);
    const auto first = pat.begin();
    const auto last = pat.end();
    const auto firstNotAny = std::find_if(first, last, &NotAnyByte);
    Require(firstNotAny != last);
    const auto lastNotAny = std::find_if(pat.rbegin(), pat.rend(), &NotAnyByte).base();
    const std::size_t offset = std::distance(first, firstNotAny);
    pat.erase(lastNotAny, pat.end());
    pat.erase(pat.begin(), firstNotAny);
    return MakePtr<LinearExpression>(offset, std::move(pat));
  }
}  // namespace Binary::FormatDSL
