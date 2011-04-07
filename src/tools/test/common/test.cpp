#include <byteorder.h>
#include <template.h>
#include <template_tools.h>
#include <parameters.h>
#include <range_checker.h>
#include <messages_collector.h>
#include <detector.h>
#include <tools.h>

#include <iostream>
#include <boost/bind.hpp>

namespace
{
  template<class T>
  void TestOrder(T orig, T test)
  {
    const T result = swapBytes(orig);
    if (result == test)
    {
      std::cout << "Passed test for " << typeid(orig).name() << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << typeid(orig).name() <<
        " has=0x" << std::hex << result << " expected=0x" << std::hex << test << std::endl;
      throw 1;
    }
  }
  
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
  
  template<class Policy>
  class FieldsSourceFromMap : public Policy
  {
  public:
    explicit FieldsSourceFromMap(const StringMap& map)
      : Map(map)
    {
    }
    
    virtual String GetFieldValue(const String& name) const
    {
      const StringMap::const_iterator it = Map.find(name);
      return it == Map.end() ? Policy::GetFieldValue(name) : it->second;
    }
  private:
    const StringMap& Map;
  };
  
  template<class Policy>
  void TestTemplate(const String& templ, const StringMap& params, const String& reference)
  {
    const FieldsSourceFromMap<Policy> source(params);
    const String res = InstantiateTemplate(templ, source);
    if (res == reference)
    {
      std::cout << "Passed test for '" << templ << '\'' << std::endl;
    }
    else
    {
      std::cout << "Failed test for '" << templ << "' (result is '" << res << "')" << std::endl;
      throw 1;
    }
  }

  enum AreaTypes
  {
    BEGIN,
    SECTION1,
    SECTION2,
    UNSPECIFIED,
    END,
  };
  typedef AreaController<AreaTypes, 1 + END, uint_t> Areas;

  void TestAreaSizes(const std::string& test, const Areas& area, uint_t bsize, uint_t s1size, uint_t s2size, uint_t esize)
  {
    Test(test + ": size of begin", area.GetAreaSize(BEGIN), bsize);
    Test(test + ": size of sect1", area.GetAreaSize(SECTION1), s1size);
    Test(test + ": size of sect2", area.GetAreaSize(SECTION2), s2size);
    Test(test + ": size of end", area.GetAreaSize(END), esize);
    Test(test + ": size of unspec", area.GetAreaSize(UNSPECIFIED), uint_t(0));
  }

  void TestDetector(const std::string& test, const std::string& pattern, bool matched, std::size_t lookAhead)
  {
    static const uint8_t SAMPLE[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    const DataFormat::Ptr format = DataFormat::Create(pattern);
    Test("Check for " + test + " match", matched == format->Match(SAMPLE, ArraySize(SAMPLE)));
    const std::size_t lookahead = format->Search(SAMPLE, ArraySize(SAMPLE));
    Test("Check for " + test + " lookahead", lookahead == lookAhead);
  }
}

int main()
{
  try
  {
  std::cout << "sizeof(int_t)=" << sizeof(int_t) << std::endl;
  std::cout << "sizeof(uint_t)=" << sizeof(uint_t) << std::endl;
  std::cout << "---- Test for byteorder working ----" << std::endl;
  TestOrder<int16_t>(-30875, 0x6587);
  TestOrder<uint16_t>(0x1234, 0x3412);
  //0x87654321
  TestOrder<int32_t>(-2023406815, 0x21436587);
  TestOrder<uint32_t>(0x12345678, 0x78563412);
  //0xfedcab9876543210
  TestOrder<int64_t>(-INT64_C(81985529216486896), UINT64_C(0x1032547698badcfe));
  TestOrder<uint64_t>(UINT64_C(0x123456789abcdef0), UINT64_C(0xf0debc9a78563412));

  std::cout << "---- Test for messages collector ----" << std::endl;
  {
    Log::MessagesCollector::Ptr msg = Log::MessagesCollector::Create();
    Test("Empty messages", !msg->CountMessages() && msg->GetMessages('\n') == String());
    msg->AddMessage("1");
    Test("Single message", msg->CountMessages() && msg->GetMessages('\n') == "1");
    msg->AddMessage("2");
    Test("Multiple message", msg->CountMessages() && msg->GetMessages('\n') == "1\n2");
  }
  
  std::cout << "---- Test for string template ----" << std::endl;
  {
    TestTemplate<SkipFieldsSource>("without template", StringMap(), "without template");
    TestTemplate<KeepFieldsSource<'[', ']'> >("no [mapped] template", StringMap(), "no [mapped] template");
    TestTemplate<SkipFieldsSource>("no [nonexisting] template", StringMap(), "no  template");
    TestTemplate<FillFieldsSource>("no [tabulated] template", StringMap(), "no             template");
    StringMap params;
    params["name"] = "value";
    TestTemplate<SkipFieldsSource>("single [name] test", params, "single value test");
    TestTemplate<SkipFieldsSource>("duplicate [name] and [name] test", params, "duplicate value and value test");
    params["value"] = "name";
    TestTemplate<SkipFieldsSource>("multiple [name] and [value] test", params, "multiple value and name test");
    TestTemplate<SkipFieldsSource>("syntax error [name test", params, "syntax error [name test");
    TestTemplate<SkipFieldsSource>("[name] at the beginning", params, "value at the beginning");
    TestTemplate<SkipFieldsSource>("at the end [name]", params, "at the end value");
  }

  std::cout << "---- Test for ranges map ----" << std::endl;
  {
    std::cout << "  test for simple range checker" << std::endl;
    RangeChecker::Ptr checker(RangeChecker::Create(100));
    Test("Adding too big range", !checker->AddRange(0, 110));
    Test("Adding range to begin", checker->AddRange(30, 10));//30..40
    Test("Adding range to lefter", checker->AddRange(20, 10));//=>20..40
    Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 40));
    Test("Adding range to end", checker->AddRange(70, 10));//70..80
    Test("Adding range to righter", checker->AddRange(80, 10));//=>70..90
    Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
    Test("Adding wrong range from left", !checker->AddRange(30, 30));
    Test("Adding wrong range to right", !checker->AddRange(50, 30));
    Test("Adding wrong range to both", !checker->AddRange(30, 50));
    Test("Adding merged to left", checker->AddRange(40, 10));//40..50 => 20..50
    Test("Adding merged to right", checker->AddRange(60, 10));//60..70 => 6..90
    Test("Adding merged to mid", checker->AddRange(50, 10));
    Test("Adding zero-sized to free", checker->AddRange(0, 0));
    Test("Adding zero-sized to busy", !checker->AddRange(30, 0));
    Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
    std::cout << "  test for shared range checker" << std::endl;
    RangeChecker::Ptr shared(RangeChecker::CreateShared(100));
    Test("Adding too big range", !shared->AddRange(0, 110));
    Test("Adding zero-sized to free", shared->AddRange(20, 0));
    Test("Adding range to begin", shared->AddRange(20, 20));//20..40
    Test("Check result", shared->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 40));
    Test("Adding range to end", shared->AddRange(70, 20));//70..90
    Test("Check result", shared->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
    Test("Adding wrong range from left", !shared->AddRange(30, 30));
    Test("Adding wrong range to right", !shared->AddRange(50, 30));
    Test("Adding wrong range to both", !shared->AddRange(30, 50));
    Test("Adding shared to left", shared->AddRange(20, 20));
    Test("Adding invalid shared to left", !shared->AddRange(20, 30));
    Test("Adding shared to right", shared->AddRange(70, 20));
    Test("Adding invalid shared to right", !shared->AddRange(60, 20));
    Test("Adding zero-sized to busy valid", shared->AddRange(70, 0));
    Test("Adding zero-sized to busy invalid", !shared->AddRange(80, 0));
    Test("Check result", shared->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
  }
  std::cout << "---- Test for area controller ----" << std::endl;
  {
    {
      Areas areas;
      areas.AddArea(BEGIN, 0);
      areas.AddArea(SECTION1, 10);
      areas.AddArea(SECTION2, 20);
      areas.AddArea(END, 30);
      TestAreaSizes("Ordered", areas, 10, 10, 10, Areas::Undefined);
    }
    {
      Areas areas;
      areas.AddArea(BEGIN, 40);
      areas.AddArea(SECTION1, 30);
      areas.AddArea(SECTION2, 20);
      areas.AddArea(END, 10);
      TestAreaSizes("Reversed", areas, Areas::Undefined, 10, 10, 10);
    }
    {
      Areas areas;
      areas.AddArea(BEGIN, 0);
      areas.AddArea(SECTION1, 30);
      areas.AddArea(SECTION2, 30);
      areas.AddArea(END, 10);
      TestAreaSizes("Duplicated", areas, 10, 0, 0, 20);
    }
  }
  std::cout << "---- Test for format detector ----" << std::endl;
  {
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
  }
  }
  catch (int code)
  {
    return code;
  }
}
