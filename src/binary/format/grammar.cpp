/**
 *
 * @file
 *
 * @brief  Format grammar
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "binary/format/grammar.h"
// common includes
#include <make_ptr.h>
#include <pointers.h>
// std includes
#include <cctype>

namespace Binary::FormatDSL
{
  const auto HEX_TOKENS = "xX0123456789abcdefABCDEF"sv;
  const auto HEXDIGITS = HEX_TOKENS.substr(2);
  const auto DIGITS = HEXDIGITS.substr(0, 10);

  class SpaceDelimitersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const auto SPACES = " \n\t\r"sv;
      return lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(SPACES) ? LexicalAnalysis::INVALID_TOKEN
                                                                               : DELIMITER;
    }
  };

  class SymbolDelimitersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const StringView DELIMITERS(&DELIMITER_TEXT, 1);
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(DELIMITERS) ? LexicalAnalysis::INVALID_TOKEN
                                                                                       : DELIMITER;
    }
  };

  class DecimalNumbersTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      return lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(DIGITS) ? LexicalAnalysis::INVALID_TOKEN
                                                                               : CONSTANT;
    }
  };

  class CharacterTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      return lexeme.empty() || lexeme[0] != SYMBOL_TEXT
                 ? LexicalAnalysis::INVALID_TOKEN
                 : (2 == lexeme.size() ? CONSTANT : LexicalAnalysis::INCOMPLETE_TOKEN);
    }
  };

  class AnyMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const StringView MASKS(&ANY_BYTE_TEXT, 1);
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(MASKS) ? LexicalAnalysis::INVALID_TOKEN
                                                                                  : MASK;
    }
  };

  class BinaryMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const auto BITMATCHES = HEX_TOKENS.substr(0, 4);
      static const auto ANY_BITS = BITMATCHES.substr(0, 2);
      const std::size_t SIZE = 9;
      if (lexeme.empty() || lexeme[0] != BINARY_MASK_TEXT || lexeme.npos != lexeme.find_first_not_of(BITMATCHES, 1)
          || lexeme.size() > SIZE)
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (lexeme.size() == SIZE)
      {
        return lexeme.npos == lexeme.find_first_of(ANY_BITS) ? CONSTANT : MASK;
      }
      else
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
    }
  };

  class HexadecimalMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const auto ANY_NIBBLES = HEX_TOKENS.substr(0, 2);
      if (lexeme.empty() || lexeme.npos != lexeme.find_first_not_of(HEX_TOKENS))
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (0 == lexeme.size() % 2)
      {
        return lexeme.npos == lexeme.find_first_of(ANY_NIBBLES) ? CONSTANT : MASK;
      }
      else
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
    }
  };

  class MultiplicityMaskTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      if (lexeme.empty() || lexeme[0] != MULTIPLICITY_TEXT)
      {
        return LexicalAnalysis::INVALID_TOKEN;
      }
      else if (lexeme.size() == 1)
      {
        return LexicalAnalysis::INCOMPLETE_TOKEN;
      }
      else
      {
        return lexeme.npos != lexeme.find_first_not_of(DIGITS, 1) ? LexicalAnalysis::INVALID_TOKEN : MASK;
      }
    }
  };

  class OperationTokenizer : public LexicalAnalysis::Tokenizer
  {
  public:
    LexicalAnalysis::TokenType Parse(StringView lexeme) const override
    {
      static const Char OPERATIONS_STR[] = {RANGE_TEXT,  CONJUNCTION_TEXT, DISJUNCTION_TEXT, QUANTOR_BEGIN,
                                            QUANTOR_END, GROUP_BEGIN,      GROUP_END};
      static const StringView OPERATIONS(OPERATIONS_STR, sizeof(OPERATIONS_STR));
      return lexeme.size() != 1 || lexeme.npos != lexeme.find_first_not_of(OPERATIONS) ? LexicalAnalysis::INVALID_TOKEN
                                                                                       : OPERATION;
    }
  };

  class FormatGrammar : public LexicalAnalysis::Grammar
  {
  public:
    FormatGrammar()
      : Delegate(LexicalAnalysis::CreateContextIndependentGrammar())
    {
      Delegate->AddTokenizer(MakePtr<SpaceDelimitersTokenizer>());
      Delegate->AddTokenizer(MakePtr<SymbolDelimitersTokenizer>());
      Delegate->AddTokenizer(MakePtr<DecimalNumbersTokenizer>());
      Delegate->AddTokenizer(MakePtr<CharacterTokenizer>());
      Delegate->AddTokenizer(MakePtr<AnyMaskTokenizer>());
      Delegate->AddTokenizer(MakePtr<BinaryMaskTokenizer>());
      Delegate->AddTokenizer(MakePtr<HexadecimalMaskTokenizer>());
      Delegate->AddTokenizer(MakePtr<MultiplicityMaskTokenizer>());
      Delegate->AddTokenizer(MakePtr<OperationTokenizer>());
    }

    void AddTokenizer(LexicalAnalysis::Tokenizer::Ptr tokenizer) override
    {
      return Delegate->AddTokenizer(std::move(tokenizer));
    }

    void Analyse(StringView notation, LexicalAnalysis::Grammar::Callback& cb) const override
    {
      return Delegate->Analyse(notation, cb);
    }

  private:
    const LexicalAnalysis::Grammar::RWPtr Delegate;
  };
}  // namespace Binary::FormatDSL

namespace Binary::FormatDSL
{
  LexicalAnalysis::Grammar::Ptr CreateFormatGrammar()
  {
    static FormatGrammar grammar;
    return MakeSingletonPointer(grammar);
  }
}  // namespace Binary::FormatDSL
