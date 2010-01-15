#include <byteorder.h>
#include <template.h>
#include <parameters.h>
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
}

int main()
{
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
      Test("m2p convert size", output.size() == result.size());
      const std::pair<Parameters::Map::iterator, Parameters::Map::iterator> compared = 
        std::mismatch(input.begin(), input.end(), result.begin(), CompareMap);
      Test("m2p convert result", compared.first == input.end());
    }
  }
}
