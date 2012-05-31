/*
Abstract:
  Format detector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <contract.h>
#include <iterator.h>
#include <tools.h>
#include <types.h>
//library includes
#include <binary/format.h>
//std includes
#include <bitset>
#include <cassert>
#include <cctype>
#include <limits>
#include <stack>
#include <stdexcept>
#include <vector>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Binary;

  const char ANY_BYTE_TEXT = '?';
  const char ANY_NIBBLE_TEXT = 'x';
  const char SKIP_BYTES_TEXT = '+';
  const char BINARY_MASK_TEXT = '%';
  const char SYMBOL_TEXT = '\'';
  const char MULTIPLICITY_TEXT = '*';

  const char RANGE_TEXT = '-';
  const char CONJUNCTION_TEXT = '&';
  const char DISJUNCTION_TEXT = '|';

  const char QUANTOR_BEGIN = '{';
  const char QUANTOR_END = '}';
  const char GROUP_BEGIN = '(';
  const char GROUP_END = ')';

  const char ANY_BIT_TEXT = 'x';
  const char ZERO_BIT_TEXT = '0';
  const char ONE_BIT_TEXT = '1';

  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  class Token
  {
  public:
    typedef boost::shared_ptr<const Token> Ptr;
    virtual ~Token() {}

    virtual bool Match(uint_t val) const = 0;
  };

  class AnyValueToken : public Token
  {
  public:
    virtual bool Match(uint_t /*val*/) const
    {
      return true;
    }

    static Ptr Create()
    {
      return boost::make_shared<AnyValueToken>();
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
      return boost::make_shared<MatchValueToken>(val);
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
        return boost::make_shared<MatchValueToken>(value);
      default:
        return boost::make_shared<MatchMaskToken>(mask, value);
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
      return boost::make_shared<MatchMultiplicityToken>(val);
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
      return in_range(val, From, To);
    }

    static Ptr Create(Ptr lh, Ptr rh)
    {
      const uint_t left = GetSingleMatchedValue(*lh);
      const uint_t right = GetSingleMatchedValue(*rh);
      Require(left < right);
      return boost::make_shared<MatchRangeToken>(left, right);
    }
  private:
    const uint_t From;
    const uint_t To;
  };

  struct Conjunction
  {
    bool operator() (bool lh, bool rh) const
    {
      return lh && rh;
    }
  };

  struct Disjunction
  {
    bool operator() (bool lh, bool rh) const
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
      , Op()
    {
    }

    virtual bool Match(uint_t val) const
    {
      return Op(Lh->Match(val), Rh->Match(val));
    }

    static Ptr Create(Ptr lh, Ptr rh)
    {
      return Ptr(new BinaryOperationToken(lh, rh));
    }
  private:
    const Ptr Lh;
    const Ptr Rh;
    const BinOp Op;
  };

  inline std::size_t ParseSkipBytes(PatternIterator& it)
  {
    Require(it && SKIP_BYTES_TEXT == *it);
    const std::size_t skip = ParseNumber(++it);
    Require(it && SKIP_BYTES_TEXT == *it);
    ++it;
    return skip;
  }

  inline std::size_t ParseQuantor(PatternIterator& it)
  {
    Require(it && QUANTOR_BEGIN == *it);
    const std::size_t mult = ParseNumber(++it);
    Require(it && QUANTOR_END == *it);
    ++it;
    return mult;
  }

  Token::Ptr ParseSingleToken(PatternIterator& it)
  {
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

  Token::Ptr ParseComplexToken(PatternIterator& it)
  {
    const Token::Ptr lh = ParseSingleToken(it);
    if (!it)
    {
      return lh;
    }
    switch (*it)
    {
    case RANGE_TEXT:
      {
        const Token::Ptr rh = ParseSingleToken(++it);
        const Token::Ptr res = MatchRangeToken::Create(lh, rh);
        Require(!IsAnyByte(res));
        return res;
      }
      break;
    case CONJUNCTION_TEXT:
      {
        const Token::Ptr rh = ParseComplexToken(++it);
        const Token::Ptr res = BinaryOperationToken<Conjunction>::Create(lh, rh);
        return res;
      }
      break;
    case DISJUNCTION_TEXT:
      {
        const Token::Ptr rh = ParseComplexToken(++it);
        const Token::Ptr res = BinaryOperationToken<Disjunction>::Create(lh, rh);
        Require(!IsAnyByte(res));
        return res;
      }
      break;
    }
    return lh;
  }

  typedef std::vector<Token::Ptr> Pattern;

  void CompilePattern(const std::string& textPattern, Pattern& compiled)
  {
    std::stack<std::size_t> groupBegins;
    std::stack<std::pair<std::size_t, std::size_t> > groups;
    Pattern result;
    for (PatternIterator it(textPattern.begin(), textPattern.end()); it;)
    {
      switch (*it)
      {
      case SKIP_BYTES_TEXT:
        {
          const std::size_t skip = ParseSkipBytes(it);
          std::fill_n(std::back_inserter(result), skip, AnyValueToken::Create());
        }
        break;
      case QUANTOR_BEGIN:
        {
          Require(!result.empty());
          if (const std::size_t mult = ParseQuantor(it))
          {
            Pattern dup;
            if (!groups.empty() && groups.top().second == result.size())
            {
              dup.assign(result.begin() + groups.top().first, result.end());
              groups.pop();
            }
            else
            {
              dup.push_back(result.back());
            }
            for (std::size_t idx = 0; idx < mult - 1; ++idx)
            {
              std::copy(dup.begin(), dup.end(), std::back_inserter(result));
            }
          }
        }
        break;
      case GROUP_BEGIN:
        groupBegins.push(result.size());
        ++it;
        break;
      case GROUP_END:
        {
          Require(!groupBegins.empty());
          const std::pair<std::size_t, std::size_t> newGroup = std::make_pair(groupBegins.top(), result.size());
          groupBegins.pop();
          groups.push(newGroup);
          ++it;
        }
        break;
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        ++it;
        break;
      default:
        {
          const Token::Ptr tok = ParseComplexToken(it);
          Require(!IsNoByte(tok));
          result.push_back(tok);
        }
      }
    }
    compiled.swap(result);
  }

  class FastSearchFormat : public Format
  {
  public:
    typedef boost::array<uint8_t, 256> PatternRow;
    typedef std::vector<PatternRow> PatternMatrix;

    FastSearchFormat(const PatternMatrix& mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.size() + offset))
      , Pat(mtx.rbegin(), mtx.rend())
      , PatRBegin(&Pat[0])
      , PatREnd(PatRBegin + Pat.size())
    {
    }

    virtual bool Match(const void* data, std::size_t size) const
    {
      if (size < MinSize)
      {
        return false;
      }
      const std::size_t endOfPat = Offset + Pat.size();
      const uint8_t* typedDataLast = static_cast<const uint8_t*>(data) + endOfPat - 1;
      return 0 == SearchBackward(typedDataLast);
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (size < MinSize)
      {
        return size;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data);
      const std::size_t endOfPat = Offset + Pat.size();
      const uint8_t* const scanStart = typedData + endOfPat - 1;
      const uint8_t* const scanStop = typedData + size;
      for (const uint8_t* scanPos = scanStart; scanPos < scanStop; )
      {
        if (const std::size_t offset = SearchBackward(scanPos))
        {
          scanPos += offset;
        }
        else
        {
          return scanPos - scanStart;
        }
      }
      return size;
    }

    static Ptr Create(Pattern::const_iterator from, Pattern::const_iterator to, std::size_t offset, std::size_t minSize)
    {
      //Pass1
      //matrix[pos][char] = Pattern.At(pos).IsMatchedFor(char)
      PatternMatrix tmp;
      for (Pattern::const_iterator it = from; it != to; ++it)
      {
        PatternRow row;
        for (uint_t idx = 0; idx != 256; ++idx)
        {
          row[idx] = (*it)->Match(idx);
        }
        tmp.push_back(row);
      }
      //Pass2
      //matrix[pos][char] = delta, Pattern.At(pos - delta).IsMatchedFor(char)
      for (uint_t idx = 0; idx != 256; ++idx)
      {
        std::size_t offset = 1;//default offset
        for (PatternMatrix::iterator it = tmp.begin(), lim = tmp.end(); it != lim; ++it, ++offset)
        {
          PatternRow& row = *it;
          if (row[idx])
          {
            offset = 0;
          }
          Require(offset <= std::numeric_limits<PatternRow::value_type>::max());
          row[idx] = static_cast<PatternRow::value_type>(offset);
        }
      }
      return boost::make_shared<FastSearchFormat>(tmp, offset, minSize);
    }
  private:
    std::size_t SearchBackward(const uint8_t* data) const
    {
      for (const PatternRow* it = PatRBegin; it != PatREnd; ++it, --data)
      {
        const PatternRow& row = *it;
        if (std::size_t offset = row[*data])
        {
          return offset;
        }
      }
      return 0;
    }
  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const PatternMatrix Pat;
    const PatternRow* const PatRBegin;
    const PatternRow* const PatREnd;
  };
}

namespace Binary
{
  Format::Ptr Format::Create(const std::string& pattern, std::size_t minSize)
  {
    Pattern pat;
    CompilePattern(pattern, pat);
    //Require(pat.size() <= minSize);
    const Pattern::const_iterator first = pat.begin();
    const Pattern::const_iterator last = pat.end();
    const Pattern::const_iterator firstNotAny = std::find_if(first, last, std::not1(std::ptr_fun(&IsAnyByte)));
    const std::size_t offset = std::distance(first, firstNotAny);
    Require(firstNotAny != last);
    return FastSearchFormat::Create(firstNotAny, last, offset, minSize);
  }
}
