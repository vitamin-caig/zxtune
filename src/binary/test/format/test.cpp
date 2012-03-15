#include <tools.h>
#include <types.h>
#include <binary/format.h>

#include <iostream>

namespace
{
  void Test(const String& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
      throw 1;
  }
  
  template<class T>
  void Test(const String& msg, T result, T reference)
  {
    if (result == reference)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << " (got: " << result << " expected: " << reference << ")" << std::endl;
      throw 1;
    }
  }
  
  void TestDetector(const std::string& test, const std::string& pattern, bool matched, std::size_t lookAhead)
  {
    static const uint8_t SAMPLE[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    const Binary::Format::Ptr format = Binary::Format::Create(pattern);
    Test("Check for " + test + " match", matched == format->Match(SAMPLE, ArraySize(SAMPLE)));
    const std::size_t lookahead = format->Search(SAMPLE, ArraySize(SAMPLE));
    Test("Check for " + test + " lookahead", lookahead, lookAhead);
  }

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
    Test("Check for " + test, res);
  }
}

int main()
{
  try
  {
    TestInvalid("bad nibble", "0g");
    TestInvalid("incomplete nibble", "?0");
    TestInvalid("invalid range", "0x-x0");
    TestInvalid("empty range", "05-05");
    TestInvalid("invalid multiple range", "01-02-03");
    TestDetector("whole any match", "?", true, 0);
    TestDetector("whole explicit match", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", true, 0);
    TestDetector("partial explicit match", "000102030405", true, 0);
    TestDetector("whole mask match", "?0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", true, 0);
    TestDetector("partial mask match", "00?02030405", true, 0);
    TestDetector("whole skip match", "00+30+1f", true, 0);
    TestDetector("partial skip match", "00+4+05", true, 0);
    TestDetector("full oversize unmatch", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", false, 32);
    TestDetector("matched from 1", "01", false, 1);
    TestDetector("matched from 7", "07", false, 7);
    TestDetector("fully unmatched", "0706", false, 32);
    TestDetector("nibbles matched", "0x0x", true, 0);
    TestDetector("nibbles unmatched", "0x1x", false, 15);
    TestDetector("binary matched", "%0x0x0x0x%x0x0x0x1", true, 0);
    TestDetector("binary unmatched", "%00010xxx%00011xxx", false, 0x17);
    TestDetector("ranged matched", "00-0200-02", true, 0);
    TestDetector("ranged unmatched", "10-12", false, 0x10);
    TestDetector("symbol unmatched", "'a'b'c'd'e", false, 32);
    TestDetector("matched from 1 with skip at begin", "?0203", false, 1);
    TestDetector("unmatched with skip at begin", "?0302", false, 32);
    TestDetector("partially matched at end", "1d1e1f20", false, 32);
    TestDetector("partially matched at end with skipping", "?1d1e1f202122", false, 32);
    TestDetector("quantor matched", "0x{10}", true, 0);
    TestDetector("quantor unmatched", "1x{15}", false, 16);
    TestDetector("quanted group matched", "(%xxxxxxx0%xxxxxxx1){3}", true, 0);
    TestDetector("quanted group unmatched", "(%xxxxxxx1%xxxxxxx0){5}", false, 1);
    TestDetector("multiplicity matched", "00010203 *2 05 *3", true, 0);
    TestDetector("multiplicity unmatched", "*2*3*4", false, 2);
  }
  catch (int code)
  {
    return code;
  }
}
