#include <tools.h>
#include <types.h>
#include <binary/format.h>
#include <binary/src/format_grammar.h>
#include <binary/src/format_syntax.h>
#include <sstream>
#include <iostream>

namespace
{
  void Test(const std::string& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
      throw 1;
  }
  
  template<class T>
  void Test(const std::string& msg, T result, T reference)
  {
    if (result == reference)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << " (got: '" << result << "' expected: '" << reference << "')" << std::endl;
      throw 1;
    }
  }
}

namespace
{  
  const std::string TOKENS("DCMO");

  class GrammarReportCallback : public LexicalAnalysis::Grammar::Callback
  {
  public:
    explicit GrammarReportCallback(std::ostream& str)
      : Str(str)
    {
    }

    virtual void TokenMatched(const std::string& lexeme, LexicalAnalysis::TokenType type)
    {
      Str << TOKENS[type] << '(' << lexeme << ") ";
    }

    virtual void MultipleTokensMatched(const std::string& lexeme, const std::set<LexicalAnalysis::TokenType>& types)
    {
      Str << "X(" << lexeme << ") ";
    }

    virtual void AnalysisError(const std::string& notation, std::size_t position)
    {
      Str << notation.substr(0, position) << " >" << notation.substr(position);
    }
  private:
    std::ostream& Str;
  };

  class SyntaxReportCallback : public Binary::FormatTokensVisitor
  {
  public:
    explicit SyntaxReportCallback(std::ostream& str)
      : Str(str)
    {
    }

    virtual void Value(const std::string& val)
    {
      Str << val << ' ';
    }

    virtual void GroupStart()
    {
      Str << "( ";
    }

    virtual void GroupEnd()
    {
      Str << ") ";
    }

    virtual void Quantor(uint_t count)
    {
      Str << '{' << count << "} ";
    }

    virtual void Operation(const std::string& op)
    {
      Str << op << ' ';
    }
  private:
    std::ostream& Str;
  };

  std::string GetGrammar(const std::string& notation)
  {
    const LexicalAnalysis::Grammar::Ptr grammar = Binary::CreateFormatGrammar();
    std::ostringstream result;
    GrammarReportCallback cb(result);
    grammar->Analyse(notation, cb);
    return result.str();
  }

  std::string GetSyntax(const std::string& notation)
  {
    std::ostringstream result;
    try
    {
      SyntaxReportCallback cb(result);
      Binary::ParseFormatNotation(notation, cb);
    }
    catch (const std::exception&)
    {
    }
    return result.str();
  }
}

namespace
{
  void TestInvalid(const std::string& test, const std::string& pattern)
  {
    bool res = false;
    try
    {
      Binary::Format::Create(pattern);
    }
    catch (const std::exception&)
    {
      res = true;
    }
    Test("invalid " + test, res);
  }
}

namespace
{
  typedef std::pair<bool, std::size_t> FormatResult;

  const FormatResult INVALID_FORMAT(false, 0);

  struct FormatTest
  {
    std::string Name;
    std::string Notation;
    std::string GrammarReport;
    std::string SyntaxReport;
    FormatResult Result;
  };

  const FormatTest TESTS[] =
  {
    //invalid grammar/syntax
    {
      "invalid char",
      "@",
      " >@",
      "",
      INVALID_FORMAT
    },
    {
      "invalid hexvalue",
      "abc",
      "C(ab) ab >c",
      "ab ",
      INVALID_FORMAT
    },
    {
      "invalid binvalue",
      "%10101102",
      " >%10101102",
      "",
      INVALID_FORMAT
    },
    {
      "invalid value",
      "123z",
      "C(123) 123 >z",
      "",
      INVALID_FORMAT
    },
    {
      "bad nibble",
      "0g",
      "C(0) 0 >g",
      "",
      INVALID_FORMAT
    },
    {
      "incomplete nibble",
      "\?0",
      "M(\?) C(0) ",
      "\? ",
      INVALID_FORMAT
    },
    //invalid format
    {
      "single delimiter",
      " ",
      "D( ) ",
      "",
      INVALID_FORMAT
    },
    {
      "multichar delimiter",
      " \t",
      "D( \t) ",
      "",
      INVALID_FORMAT
    },
    {
      "decimal constant",
      "10",
      "C(10) ",
      "10 ",
      FormatResult(false, 0x10)
    },
    {
      "hexadecimal constant",
      "af",
      "C(af) ",
      "af ",
      FormatResult(false, 32)
    },
    {
      "binary constant",
      "%01010101",
      "C(%01010101) ",
      "%01010101 ",
      FormatResult(false, 32)
    },
    {
      "char constant",
      "'A",
      "C('A) ",
      "'A ",
      FormatResult(false, 32)
    },
    {
      "any mask",
      "\?",
      "M(\?) ",
      "\? ",
      INVALID_FORMAT
    },
    {
      "hex mask",
      "2x",
      "M(2x) ",
      "2x ",
      FormatResult(false, 32)
    },
    {
      "bin mask",
      "%01xx1010",
      "M(%01xx1010) ",
      "%01xx1010 ",
      FormatResult(false, 32)
    },
    {
      "mult mask",
      "*5",
      "M(*5) ",
      "*5 ",
      FormatResult(true, 0)
    },
    {
      "operations",
      "-&|{}()",
      "O(-) O(&) O(|) O({) O(}) O(() O()) ",
      "- & | ",
      INVALID_FORMAT,
    },
    {
      "single char delimiters",
      ",,",
      "D(,) D(,) ",
      "",
      INVALID_FORMAT,
    },
    {
      "invalid range",
      "0x-x0",
      "M(0x) O(-) M(x0) ",
      "0x - x0 ",
      INVALID_FORMAT
    },
    {
      "empty range",
      "05-05",
      "C(05) O(-) C(05) ",
      "05 - 05 ",
      INVALID_FORMAT,
    },
    {
      "full range",
      "00-ff",
      "C(00) O(-) C(ff) ",
      "00 - ff ",
      INVALID_FORMAT
    },
    {
      "invalid multiple range",
      "01-02-03",
      "C(01) O(-) C(02) O(-) C(03) ",
      "01 - 02 - 03 ",
      INVALID_FORMAT
    },
    {
      "empty conjunction",
      "01&10",
      "C(01) O(&) C(10) ",
      "01 & 10 ",
      INVALID_FORMAT
    },
    {
      "full disjunction",
      "00-80|80-ff",
      "C(00) O(-) C(80) O(|) C(80) O(-) C(ff) ",
      "00 - 80 | 80 - ff ",
      INVALID_FORMAT
    },
    //valid tests
    {
      "whole explicit match",
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
      "C(000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f) ",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f ",
      FormatResult(true, 0)
    },
    {
      "partial explicit match",
      "000102030405",
      "C(000102030405) ",
      "00 01 02 03 04 05 ",
      FormatResult(true, 0)
    },
    {
      "whole mask match",
      "\?0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
      "M(\?) C(0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f) ",
      "\? 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f ",
      FormatResult(true, 0)
    },
    {
      "partial mask match",
      "00\?02030405",
      "C(00) M(\?) C(02030405) ",
      "00 \? 02 03 04 05 ",
      FormatResult(true, 0)
    },
    {
      "full oversize unmatch",
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20",
      "C(000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20) ",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 ",
      FormatResult(false, 32)
    },
    {
      "nibbles matched",
      "0x0x",
      "M(0x0x) ",
      "0x 0x ",
      FormatResult(true, 0)
    },
    {
      "nibbles unmatched",
      "0x1x",
      "M(0x1x) ",
      "0x 1x ",
      FormatResult(false, 15)
    },
    {
      "binary matched",
      "%0x0x0x0x%x0x0x0x1", 
      "M(%0x0x0x0x) M(%x0x0x0x1) ",
      "%0x0x0x0x %x0x0x0x1 ",
      FormatResult(true, 0)
    },
    {
      "binary unmatched",
      "%00010xxx%00011xxx",
      "M(%00010xxx) M(%00011xxx) ",
      "%00010xxx %00011xxx ",
      FormatResult(false, 0x17)
    },
    {
      "ranged matched",
      "00-0200-02",
      "C(00) O(-) C(0200) O(-) C(02) ",
      "00 - 02 00 - 02 ",
      FormatResult(true, 0)
    },
    {
      "ranged unmatched",
      "10-12",
      "C(10) O(-) C(12) ",
      "10 - 12 ",
      FormatResult(false, 0x10)
    },
    {
      "symbol unmatched",
      "'a'b'c'd'e",
      "C('a) C('b) C('c) C('d) C('e) ",
      "'a 'b 'c 'd 'e ",
      FormatResult(false, 32)
    },
    {
      "matched from 1 with skip at begin",
      "\?0203",
      "M(\?) C(0203) ",
      "\? 02 03 ",
      FormatResult(false, 1)
    },
    {
      "unmatched with skip at begin",
      "\?0302", 
      "M(\?) C(0302) ",
      "\? 03 02 ",
      FormatResult(false, 32)
    },
    {
      "matched from 13 with skip at end",
      "0d0e\?\?111213\?\?", 
      "C(0d0e) M(\?) M(\?) C(111213) M(\?) M(\?) ",
      "0d 0e \? \? 11 12 13 \? \? ",
      FormatResult(false, 13)
    },
    {
      "unmatched with skip at end",
      "0302\?\?", 
      "C(0302) M(\?) M(\?) ",
      "03 02 \? \? ",
      FormatResult(false, 32)
    },
    {
      "partially matched at end",
      "1d1e1f20",
      "C(1d1e1f20) ",
      "1d 1e 1f 20 ",
      FormatResult(false, 32)
    },
    {
      "partially matched at end with skipping",
      "\?1d1e1f202122",
      "M(\?) C(1d1e1f202122) ",
      "\? 1d 1e 1f 20 21 22 ",
      FormatResult(false, 32)
    },
    {
      "quantor matched",
      "0x{10}",
      "M(0x) O({) C(10) O(}) ",
      "0x {10} ",
      FormatResult(true, 0)
    },
    {
      "quantor unmatched",
      "1x{15}",
      "M(1x) O({) C(15) O(}) ",
      "1x {15} ",
      FormatResult(false, 16)
    },
    {
      "quanted group matched",
      "(%xxxxxxx0%xxxxxxx1){3}",
      "O(() M(%xxxxxxx0) M(%xxxxxxx1) O()) O({) C(3) O(}) ",
      "( %xxxxxxx0 %xxxxxxx1 ) {3} ",
      FormatResult(true, 0)
    },
    {
      "quanted group unmatched",
      "(%xxxxxxx1%xxxxxxx0){5}",
      "O(() M(%xxxxxxx1) M(%xxxxxxx0) O()) O({) C(5) O(}) ",
      "( %xxxxxxx1 %xxxxxxx0 ) {5} ",
      FormatResult(false, 1)
    },
    {
      "multiplicity matched",
      "00010203 *2 05 *3",
      "C(00010203) D( ) M(*2) D( ) C(05) D( ) M(*3) ",
      "00 01 02 03 *2 05 *3 ",
      FormatResult(true, 0)
    },
    {
      "multiplicity unmatched",
      "*2*3*4",
      "M(*2) M(*3) M(*4) ",
      "*2 *3 *4 ",
      FormatResult(false, 2)
    },
    {
      "conjunction matched",
      "00 0x&x1",
      "C(00) D( ) M(0x) O(&) M(x1) ",
      "00 0x & x1 ",
      FormatResult(true, 0)
    },
    {
      "conjunction unmatched",
      "(1x&%xxxx11xx){4}",
      "O(() M(1x) O(&) M(%xxxx11xx) O()) O({) C(4) O(}) ",
      "( 1x & %xxxx11xx ) {4} ",
      FormatResult(false, 0x1c)
    },
    {
      "disjunction matched",
      "00|01 00|01",
      "C(00) O(|) C(01) D( ) C(00) O(|) C(01) ",
      "00 | 01 00 | 01 ",
      FormatResult(true, 0)
    },
    {
      "disjunction unmatched",
      "(1x|%xxxx11xx){6}",
      "O(() M(1x) O(|) M(%xxxx11xx) O()) O({) C(6) O(}) ",
      "( 1x | %xxxx11xx ) {6} ",
      FormatResult(false, 12)
    },
    /*
    {
      "complex condition matched",
      "00-01|1e-1f",
      "C(00) O(-) C(01) O(|) C(1e) O(-) C(1f) ",
      "00 - 01 | 1e - 1f ",
      FormatResult(true, 0)
    }
    */
  };

  void Execute(const FormatTest& tst)
  {
    std::cout << "Testing for " << tst.Name << std::endl;
    Test("grammar", GetGrammar(tst.Notation), tst.GrammarReport);
    Test("syntax", GetSyntax(tst.Notation), tst.SyntaxReport);
    if (tst.Result == INVALID_FORMAT)
    {
      TestInvalid("format creating", tst.Notation);
    }
    else
    {
      static const uint8_t SAMPLE[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
      const Binary::Format::Ptr format = Binary::Format::Create(tst.Notation);
      Test("match", format->Match(SAMPLE, ArraySize(SAMPLE)), tst.Result.first);
      const std::size_t lookahead = format->Search(SAMPLE, ArraySize(SAMPLE));
      Test("lookahead", lookahead, tst.Result.second);
    }
  }
}

int main()
{
  try
  {
    std::for_each(TESTS, ArrayEnd(TESTS), std::ptr_fun(&Execute));
  }
  catch (int code)
  {
    return code;
  }
}
