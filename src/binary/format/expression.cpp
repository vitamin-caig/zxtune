/**
 *
 * @file
 *
 * @brief  Binary expression compiler
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/format/expression.h"

#include "binary/format/grammar.h"
#include "binary/format/syntax.h"

#include "math/numeric.h"
#include "tools/iterators.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stack>
#include <utility>
#include <vector>

namespace Binary::FormatDSL
{
  using PatternIterator = RangeIterator<StringView::const_iterator>;

  class InternalPredicate : public Predicate
  {
  public:
    using Ptr = std::unique_ptr<const InternalPredicate>;
  };

  class AnyValuePredicate : public Predicate
  {
  public:
    AnyValuePredicate() = default;

    bool Match(uint_t /*val*/) const override
    {
      return true;
    }

    static const Predicate* Instance()
    {
      static const AnyValuePredicate INSTANCE;
      return &INSTANCE;
    }
  };

  class Pattern
  {
  public:
    std::size_t Size() const
    {
      return Predicates.size();
    }

    void AddVerified(InternalPredicate::Ptr pred)
    {
      Verify(*pred);
      Add(std::move(pred));
    }

    void Add(InternalPredicate::Ptr pred)
    {
      Pool.emplace_back(std::move(pred));
      Add(Pool.back().get());
    }

    void Add(const Predicate* pred)
    {
      Predicates.emplace_back(pred);
    }

    void DuplicateLast(std::size_t count)
    {
      const auto* last = Predicates.back();
      Predicates.resize(Predicates.size() + count - 1, last);
    }

    void DuplicateTail(std::size_t tailSize, std::size_t count)
    {
      const auto oldSize = Predicates.size();
      Predicates.resize(oldSize + tailSize * (count - 1));
      const auto oldEnd = Predicates.begin() + oldSize;
      // Overlapping copying is UB
      for (auto src = oldEnd - tailSize, dst = oldEnd, lim = Predicates.end(); dst != lim; ++src, ++dst)
      {
        *dst = *src;
      }
    }

    const Predicate& PopLast()
    {
      Require(!Predicates.empty());
      const auto* res = Predicates.back();
      Predicates.pop_back();
      return *res;
    }

    std::size_t Cleanup()
    {
      const auto first = Predicates.begin();
      const auto last = Predicates.end();
      const auto isAny = [](const Predicate* p) { return p == AnyValuePredicate::Instance(); };
      const auto firstNotAny = std::find_if_not(first, last, isAny);
      Require(firstNotAny != last);
      const auto lastNotAny = std::find_if_not(Predicates.rbegin(), Predicates.rend(), isAny).base();
      if (first != firstNotAny)
      {
        std::copy(firstNotAny, lastNotAny, first);
      }
      Predicates.resize(std::distance(firstNotAny, lastNotAny));
      return std::distance(first, firstNotAny);
    }

    std::span<const Predicate* const> GetPredicates() const
    {
      return {Predicates};
    }

  private:
    static void Verify(const Predicate& pred)
    {
      bool hasMatched = false;
      bool hasUnmatched = false;
      for (uint_t val = 0; val < 256; ++val)
      {
        const auto res = pred.Match(val);
        hasMatched |= res;
        hasUnmatched |= !res;
        if (hasMatched && hasUnmatched)
        {
          return;
        }
      }
      Require(false);
    }

  private:
    std::vector<InternalPredicate::Ptr> Pool;
    std::vector<const Predicate*> Predicates;
  };

  class MatchValuePredicate : public InternalPredicate
  {
  public:
    explicit MatchValuePredicate(uint_t value)
      : Value(value)
    {}

    bool Match(uint_t val) const override
    {
      return val == Value;
    }

    static InternalPredicate::Ptr Create(PatternIterator& it)
    {
      Require(it && SYMBOL_TEXT == *it && ++it);
      const char val = *it;
      ++it;
      return MakePtr<MatchValuePredicate>(val);
    }

  private:
    friend class MatchRangePredicate;
    const uint_t Value;
  };

  class MatchMaskPredicate : public InternalPredicate
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

    static void Parse(PatternIterator it, Pattern& out)
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
        return out.Add(AnyValuePredicate::Instance());
      case 0xff:
        return out.Add(MakePtr<MatchValuePredicate>(value));
      default:
        return out.Add(MakePtr<MatchMaskPredicate>(mask, value));
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

  class MatchMultiplicityPredicate : public InternalPredicate
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

  class MatchRangePredicate : public InternalPredicate
  {
  public:
    MatchRangePredicate(uint_t from, uint_t to)
      : From(from)
      , To(to)
    {
      Require(from < to);
      Require(from != 0 || to != 255);
    }

    bool Match(uint_t val) const override
    {
      return Math::InRange(val, From, To);
    }

    static Ptr Create(const Predicate& lh, const Predicate& rh)
    {
      const uint_t left = GetSingleMatchingValue(lh);
      const uint_t right = GetSingleMatchingValue(rh);
      return MakePtr<MatchRangePredicate>(left, right);
    }

  private:
    static uint_t GetSingleMatchingValue(const Predicate& pred)
    {
      const auto* const match = dynamic_cast<const MatchValuePredicate*>(&pred);
      Require(match);
      return match->Value;
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
  class BinaryOperationPredicate : public InternalPredicate
  {
  public:
    BinaryOperationPredicate(const Predicate& lh, const Predicate& rh)
      : Lh(lh)
      , Rh(rh)
    {}

    bool Match(uint_t val) const override
    {
      return BinOp::Execute(val, Lh, Rh);
    }

    static Ptr Create(const Predicate& lh, const Predicate& rh)
    {
      return MakePtr<BinaryOperationPredicate>(lh, rh);
    }

  private:
    const Predicate& Lh;
    const Predicate& Rh;
  };

  inline std::size_t ParseQuantor(PatternIterator& it)
  {
    Require(it && QUANTOR_BEGIN == *it);
    const std::size_t mult = ParseNumber(++it);
    Require(it && QUANTOR_END == *it);
    ++it;
    return mult;
  }

  void ParseSinglePredicate(StringView txt, Pattern& out)
  {
    PatternIterator it(txt.begin(), txt.end());
    Require(it);
    switch (*it)
    {
    case ANY_BYTE_TEXT:
      return out.Add(AnyValuePredicate::Instance());
    case SYMBOL_TEXT:
      return out.Add(MatchValuePredicate::Create(it));
    case MULTIPLICITY_TEXT:
      return out.Add(MatchMultiplicityPredicate::Create(it));
    case BINARY_MASK_TEXT:
    default:
      return MatchMaskPredicate::Parse(it, out);
    }
  }

  void ParseOperation(StringView txt, Pattern& pat)
  {
    Require(!txt.empty());
    const auto& rh = pat.PopLast();
    const auto& lh = pat.PopLast();
    switch (txt[0])
    {
    case RANGE_TEXT:
      return pat.Add(MatchRangePredicate::Create(lh, rh));
    case CONJUNCTION_TEXT:
      return pat.AddVerified(BinaryOperationPredicate<Conjunction>::Create(lh, rh));
    case DISJUNCTION_TEXT:
      return pat.AddVerified(BinaryOperationPredicate<Disjunction>::Create(lh, rh));
    default:
      Require(false);
      return;
    }
  }

  class PredicatesFactory : public FormatTokensVisitor
  {
  public:
    void Match(StringView val) override
    {
      ParseSinglePredicate(val, Result);
    }

    void GroupStart() override
    {
      GroupBegins.push(Result.Size());
    }

    void GroupEnd() override
    {
      Require(!GroupBegins.empty());
      Groups.emplace(GroupBegins.top(), Result.Size());
      GroupBegins.pop();
    }

    void Quantor(uint_t count) override
    {
      Require(count != 0);
      const auto avail = Result.Size();
      Require(avail != 0);
      if (!Groups.empty() && Groups.top().second == avail)
      {
        Result.DuplicateTail(avail - Groups.top().first, count);
        Groups.pop();
      }
      else
      {
        Result.DuplicateLast(count);
      }
    }

    void Operation(StringView op) override
    {
      ParseOperation(op, Result);
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
    explicit LinearExpression(Pattern pat)
      : Offset(pat.Cleanup())
      , Pat(std::move(pat))
    {
      Require(pat.Size() < 1000);
    }

    std::size_t StartOffset() const override
    {
      return Offset;
    }

    std::span<const Predicate* const> Predicates() const override
    {
      return Pat.GetPredicates();
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
    return MakePtr<LinearExpression>(std::move(pat));
  }
}  // namespace Binary::FormatDSL
