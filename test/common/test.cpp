#include <byteorder.h>
#include <template.h>
#include <parameters.h>
#include <range_checker.h>
#include <messages_collector.h>

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
    }
  }
  
  void Test(const String& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
  }
  
  void TestTemplate(const String& templ, const StringMap& params, const String& reference)
  {
    const String res = InstantiateTemplate(templ, params);
    if (res == reference)
    {
      std::cout << "Passed test for '" << templ << '\'' << std::endl;
    }
    else
    {
      std::cout << "Failed test for '" << templ << "' (result is '" << res << "')" << std::endl;
    }
  }
  
  void TestMap(const StringMap& input, const String& idx, const String& reference)
  {
    const StringMap::const_iterator it = input.find(idx);
    if (it == input.end())
    {
      std::cout << "Failed (no such key)";
    }
    else if (it->second != reference)
    {
      std::cout << "Failed (" << it->second << "!=" << reference << ")";
    }
    else
    {
      std::cout << "Passed";
    }
    std::cout << " testing for " << idx << std::endl;
  }
  
  inline bool CompareMap(const Parameters::Map::value_type& lh, const Parameters::Map::value_type& rh)
  {
    return lh.first == rh.first && lh.second == rh.second;
  }
  
  void TestMaps(const std::string& title, const Parameters::Map& result, const Parameters::Map& etalon)
  {
    Test(title + " size", result.size() == etalon.size());
    Test(title, etalon.end() == std::mismatch(etalon.begin(), etalon.end(), result.begin(), CompareMap).first);
  }
  
  void OutMap(const Parameters::Map::value_type& val)
  {
    std::cout << '[' << val.first << "]=" << Parameters::ConvertToString(val.second) << std::endl;
  }
}

int main()
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
    TestTemplate("without template", StringMap(), "without template");
    TestTemplate("no [mapped] template", StringMap(), "no [mapped] template");
    StringMap params;
    params["name"] = "value";
    TestTemplate("single [name] test", params, "single value test");
    TestTemplate("duplicate [name] and [name] test", params, "duplicate value and value test");
    params["value"] = "name";
    TestTemplate("multiple [name] and [value] test", params, "multiple value and name test");
    TestTemplate("syntax error [name test", params, "syntax error [name test");
  }
  
  std::cout << "---- Test for parameters map converting ----" << std::endl;
  {
    Dump data(3);
    data[0] = 0x12;
    data[1] = 0x34;
    data[2] = 0x56;
    {
      Parameters::Map input;
      input["integer"] = 123;
      input["string"] = "hello";
      input["data"] = data;
      input["strAsInt"] = "456";
      input["strAsData"] = "#00";
      input["strAsIntInvalid"] = "678a";
      input["strAsDataInvalid"] = "#00a";
      input["strAsStrQuoted"] = "'test'";
      StringMap output;
      Parameters::ConvertMap(input, output);
      Test("p2m convert size", input.size() == output.size());
      TestMap(output, "integer", "123");
      TestMap(output, "string", "hello");
      TestMap(output, "data", "#123456");
      TestMap(output, "strAsInt", "'456'");
      TestMap(output, "strAsData", "'#00'");
      TestMap(output, "strAsIntInvalid", "678a");
      TestMap(output, "strAsDataInvalid", "#00a");
      TestMap(output, "strAsStrQuoted", "''test''");
      Parameters::Map result;
      Parameters::ConvertMap(output, result);
      TestMaps("m2p convert", result, input);
    }
  }
  std::cout << "---- Test for parameters map processing ----" << std::endl;
  {
    Parameters::Map first;
    first["key1"] = "val1";
    first["key2"] = "val2";
    first["key3"] = "val3";
    Parameters::Map second;
    second["key1"] = "val1";
    second["key2"] = "val2_new";
    second["key4"] = "val4";
    
    Parameters::Map updInSecond;
    updInSecond["key2"] = "val2_new";
    updInSecond["key4"] = "val4";
    
    {
      Parameters::Map test;
      Parameters::DifferMaps(second, first, test);
      TestMaps("differ test", test, updInSecond);
    }
    
    Parameters::Map merged;
    merged["key1"] = "val1";
    merged["key2"] = "val2";
    merged["key3"] = "val3";
    merged["key4"] = "val4";
    {
      Parameters::Map test;
      Parameters::MergeMaps(first, second, test, false);
      TestMaps("merge keep test", test, merged);
      //std::for_each(test.begin(), test.end(), OutMap);
    }
    merged["key2"] = "val2_new";
    {
      Parameters::Map test;
      Parameters::MergeMaps(first, second, test, true);
      TestMaps("merge replace test", test, merged);
      //std::for_each(test.begin(), test.end(), OutMap);
    }
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

}
