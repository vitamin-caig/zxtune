/**
 *
 * @file
 *
 * @brief  Strings test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <strings/casing.h>
#include <strings/conversion.h>
#include <strings/encoding.h>
#include <strings/fields.h>
#include <strings/fields_filter.h>
#include <strings/format.h>
#include <strings/join.h>
#include <strings/map.h>
#include <strings/optimize.h>
#include <strings/prefixed_index.h>
#include <strings/sanitize.h>
#include <strings/split.h>
#include <strings/template.h>

#include <iostream>

template<class T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& arr)
{
  if (!arr.empty())
  {
    char delimiter = '[';
    for (const auto& e : arr)
    {
      o << delimiter << e;
      delimiter = ',';
    }
    o << ']';
  }
  else
  {
    o << "[]";
  }
  return o;
}

template<class T1, class T2, std::enable_if_t<!std::is_same_v<T1, T2>, int> = 0>
bool operator==(const std::vector<T1>& lh, const std::vector<T2>& rh)
{
  return lh.size() == rh.size() && lh.end() == std::mismatch(lh.begin(), lh.end(), rh.begin()).first;
}

namespace
{
  template<class Policy>
  class FieldsSourceFromMap : public Policy
  {
  public:
    explicit FieldsSourceFromMap(const Strings::Map& map)
      : Map(map)
    {}

    String GetFieldValue(StringView name) const override
    {
      const auto it = Map.find(name);
      return it == Map.end() ? Policy::GetFieldValue(name) : it->second;
    }

  private:
    const Strings::Map& Map;
  };

  void Test(bool result, StringView msg)
  {
    if (result)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << std::endl;
      throw 1;
    }
  }

  char ToHex(uint8_t s)
  {
    return s < 10 ? '0' + s : 'a' + s - 10;
  }

  String Escape(const StringView& in)
  {
    String out;
    out += '\'';
    for (uint8_t c : in)
    {
      if (c >= ' ' && c < 127)
      {
        out += c;
      }
      else
      {
        out += '\\';
        switch (c)
        {
        case '\\':
          out += c;
          break;
        case '\r':
          out += 'r';
          break;
        case '\n':
          out += 'n';
          break;
        case '\t':
          out += 't';
          break;
        default:
          out += 'x';
          out += ToHex(c >> 4);
          out += ToHex(c & 15);
          break;
        }
      }
    }
    out += '\'';
    return out;
  }

  String Escape(const String& in)
  {
    return Escape(StringView{in});
  }

  template<class T>
  T Escape(T in)
  {
    return in;
  }

  template<class T1, class T2>
  void TestEquals(T1 ref, T2 val, StringView msg)
  {
    if (ref == val)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << ": ref=" << Escape(ref) << " val=" << Escape(val) << std::endl;
      throw 1;
    }
  }

  void TestTemplate(const Strings::FieldsSource& source, const String& templ, StringView reference)
  {
    TestEquals(reference, Strings::Template::Instantiate(templ, source), "Template '" + templ + "'");
  }

  template<class Policy>
  void TestTemplate(const String& templ, const Strings::Map& params, StringView reference)
  {
    const FieldsSourceFromMap<Policy> source(params);
    TestTemplate(source, templ, reference);
  }

  void TestTranscode(const String& encoding, StringView str, StringView reference)
  {
    const auto& trans = Strings::ToAutoUtf8(str);
    TestEquals(reference, trans, encoding + " for " + trans);
    const auto& transTrans = Strings::ToAutoUtf8(trans);
    TestEquals(trans, transTrans, "Repeated transcode");
  }

  void TestOptimize(StringView str, const String& reference)
  {
    TestEquals(reference, Strings::OptimizeAscii(str), "Optimize " + reference);
  }

  struct SanitizeCase
  {
    StringView Title;
    StringView Input;
    StringView Ref;
    StringView RefKeepPadding;
    StringView RefMultiline;

    void Test() const
    {
      const String title(Title);
      TestEquals(Ref, Strings::Sanitize(Input), "Sanitize " + title);
      TestEquals(RefKeepPadding, Strings::SanitizeKeepPadding(Input), "SanitizeKeepPadding " + title);
      TestEquals(RefMultiline, Strings::SanitizeMultiline(Input), "SanitizeMultiline " + title);
    }
  };

  template<class T>
  void TestParse(const String& msg, const StringView str, T reference, const StringView restPart)
  {
    Test(Strings::ConvertTo<T>(str) == reference, "ConvertTo " + msg);
    {
      T ret = 123;
      Test(Strings::Parse(str, ret) == restPart.empty(), "Parse " + msg);
      Test(ret == restPart.empty() ? reference : 123, "Parse result " + msg);
    }
    {
      StringView strCopy = str;
      Test(Strings::ParsePartial<T>(strCopy) == reference, "ParsePartial " + msg);
      Test(strCopy == restPart, "ParsePartial rest " + msg);
    }
  }

  template<class T, class D>
  void TestSplitImpl(StringView msg, StringView str, D delimiter, const std::vector<StringView>& reference)
  {
    std::vector<T> out;
    Strings::Split(str, delimiter, out);
    TestEquals(reference, out, msg);
  }

  template<class D>
  void TestSplit(const String& msg, StringView str, D delimiter, const std::vector<StringView>& reference)
  {
    TestSplitImpl<StringView>(msg + " StringView", str, delimiter, reference);
    TestSplitImpl<String>(msg + " String", str, delimiter, reference);
  }

  template<class T>
  void TestJoin(const String& msg)
  {
    const auto delimiter = ","_sv;
    std::vector<T> data;
    TestEquals("", Strings::Join(data, delimiter), "Join empty vector of " + msg);
    data.emplace_back("one");
    TestEquals("one", Strings::Join(data, delimiter), "Join single element vector of " + msg);
    data.emplace_back("two");
    TestEquals("one,two", Strings::Join(data, delimiter), "Join several elements vector of " + msg);
  }
}  // namespace

int main()
{
  try
  {
    std::cout << "---- Test for string template ----" << std::endl;
    {
      TestTemplate<Strings::SkipFieldsSource>("without template", Strings::Map(), "without template");
      TestTemplate<Strings::KeepFieldsSource>("no [mapped] template", Strings::Map(), "no [mapped] template");
      TestTemplate<Strings::SkipFieldsSource>("no [nonexisting] template", Strings::Map(), "no  template");
      TestTemplate<Strings::FillFieldsSource>("no [tabulated] template", Strings::Map(), "no             template");
      Strings::Map params;
      params["name"] = "value";
      TestTemplate<Strings::SkipFieldsSource>("single [name] test", params, "single value test");
      TestTemplate<Strings::SkipFieldsSource>("duplicate [name] and [name] test", params,
                                              "duplicate value and value test");
      params["value"] = "name";
      TestTemplate<Strings::SkipFieldsSource>("multiple [name] and [value] test", params,
                                              "multiple value and name test");
      TestTemplate<Strings::SkipFieldsSource>("syntax error [name test", params, "syntax error [name test");
      TestTemplate<Strings::SkipFieldsSource>("[name] at the beginning", params, "value at the beginning");
      TestTemplate<Strings::SkipFieldsSource>("at the end [name]", params, "at the end value");
      const FieldsSourceFromMap<Strings::SkipFieldsSource> source(params);
      const Strings::FilterFieldsSource replaceToChar(source, "abcde", '%');
      TestTemplate(replaceToChar, "Replace bunch of symbols to single in [name] and [value]",
                   "Replace bunch of symbols to single in v%lu% and n%m%");
      const Strings::FilterFieldsSource replaceToCharsSet(source, "abcde", "ABCDE");
      TestTemplate(replaceToCharsSet, "Replace bunch of symbols to multiple in [name] and [value]",
                   "Replace bunch of symbols to multiple in vAluE and nAmE");
    }
    std::cout << "---- Test for transcode ----" << std::endl;
    {
      TestTranscode("CP1252", "S\xf8ren", "S\xc3\xb8ren");
      TestTranscode("CP1252", "\xd8j", "\xc3\x98j");
      TestTranscode("CP1252", "H\xfclsbeck", "H\xc3\xbclsbeck");
      TestTranscode("CP1252", "Mih\xe1ly", "Mih\xc3\xa1ly");
      TestTranscode("CP1252", "\xd6\xf6rni", "\xc3\x96\xc3\xb6rni");
      TestTranscode("CPXXXX", "720\xb0", "720\xc2\xb0");
      TestTranscode("CP1252", "M\xf6ller", "M\xc3\xb6ller");
      TestTranscode("CP1252", "Norrg\xe5rd", "Norrg\xc3\xa5rd");
      TestTranscode("CP1252", "Skarzy\xf1zki", "Skarzy\xc3\xb1zki");
      TestTranscode("CP866", "\xac\xe3\xa7\xeb\xaa\xa0", "\xd0\xbc\xd1\x83\xd0\xb7\xd1\x8b\xd0\xba\xd0\xb0");
      TestTranscode("UTF-8", "\xe3\x81\xaf\xe3\x81\x98", "\xe3\x81\xaf\xe3\x81\x98");
      TestTranscode("UTF-8 BOM", "\xef\xbb\xbf\xe3\x81\xaf\xe3\x81\x98", "\xe3\x81\xaf\xe3\x81\x98");
      TestTranscode("CP1251", "\xe4\xe5\xe4\xf3\xf8\xea\xe0",
                    "\xd0\xb4\xd0\xb5\xd0\xb4\xd1\x83\xd1\x88\xd0\xba\xd0\xb0");
      TestTranscode("SJIS", "\x83\x50\x83\x43\x83\x93\x82\xcc\x83\x65\x81\x5b\x83\x7d",
                    "\xe3\x82\xb1\xe3\x82\xa4\xe3\x83\xb3\xe3\x81\xae\xe3\x83\x86\xe3\x83\xbc\xe3\x83\x9e");
      TestTranscode("UTF-16LE", "\xff\xfe\x22\x04\x35\x04\x41\x04\x42\x04", "\xd0\xa2\xd0\xb5\xd1\x81\xd1\x82");
      TestTranscode("UTF-16BE", "\xfe\xff\x04\x22\x04\x35\x04\x41\x04\x42", "\xd0\xa2\xd0\xb5\xd1\x81\xd1\x82");
      TestTranscode("UTF-16 surrogate", "\xfe\xff\xd8\x52\xdf\x62", "\xf0\xa4\xad\xa2");
      // unpaired surrogate should be encoded into 3 utf-8 bytes
      TestTranscode("UTF-16 unpaired surrogate", "\xfe\xff\xd8\x52", "\xed\xa1\x92");
    }
    std::cout << "---- Test for optimize ----" << std::endl;
    {
      TestOptimize("One", "One");
      TestOptimize("One Two", "One Two");
      TestOptimize("\x1One", "One");
      TestOptimize("\x1\x82One", "One");
      TestOptimize("One\x1", "One");
      TestOptimize("One\x1\x82", "One");
      TestOptimize("\x1One\x82", "One");
      TestOptimize("\x1\x82One\x82\x1", "One");
      TestOptimize("\x1One\x82Two\x3", "One?Two");
      TestOptimize("\x1One\x82\x3Two\x84", "One??Two");
    }
    std::cout << "---- Test for sanitize ----" << std::endl;
    {
      constexpr SanitizeCase cases[] = {
          {"Empty", "", "", "", ""},
          {"Space", " ", "", "", ""},
          {"Tab", "\t", "", "", ""},
          {"Newline", "\n", "", "", ""},
          {"Return", "\r", "", "", ""},
          {"Dos return", "\r\n", "", "", ""},
          {"Complex", "\tTabs\r\nand\r line\nbre\xb0ks \rwith\x02spaces\r", "Tabs and  line bre\xc2\xb0ks  withspaces",
           " Tabs and  line bre\xc2\xb0ks  with spaces", "Tabs\nand\nline\nbre\xc2\xb0ks\nwithspaces"},
      };
      for (const auto& c : cases)
      {
        c.Test();
      }
    }
    std::cout << "---- Test for prefixed index ----" << std::endl;
    {
      {
        const auto composed = Strings::PrefixedIndex::Create("Prefix", 123);
        Test(composed.IsValid(), "Composed IsValid");
        Test(composed.GetIndex() == 123, "Composed GetIndex");
        Test(composed.ToString() == "Prefix123", "Composed ToString");
      }
      {
        const auto parsed = Strings::PrefixedIndex::Parse("Prefix", "Prefix456");
        Test(parsed.IsValid(), "Parsed IsValid");
        Test(parsed.GetIndex() == 456, "Parsed GetIndex");
        Test(parsed.ToString() == "Prefix456", "Parsed ToString");
      }
      {
        const auto garbage = Strings::PrefixedIndex::Parse("Prefix", "Prefix456sub");
        Test(!garbage.IsValid(), "Garbage IsValid");
        Test(garbage.GetIndex() == 0, "Garbage GetIndex");
        Test(garbage.ToString() == "Prefix456sub", "Garbage ToString");
      }
      {
        const auto invalid = Strings::PrefixedIndex::Parse("Prefix", "SomeString");
        Test(!invalid.IsValid(), "Invalid IsValid");
        Test(invalid.GetIndex() == 0, "Invalid GetIndex");
        Test(invalid.ToString() == "SomeString", "Invalid ToString");
      }
      {
        const auto empty = Strings::PrefixedIndex::Parse("Prefix", "Prefix");
        Test(!empty.IsValid(), "Empty IsValid");
        Test(empty.GetIndex() == 0, "Empty GetIndex");
        Test(empty.ToString() == "Prefix", "Empty ToString");
      }
      {
        const auto caseInvalid = Strings::PrefixedIndex::Parse("prefix", "PREFIX123");
        Test(!caseInvalid.IsValid(), "CaseInvalid IsValid");
        Test(caseInvalid.GetIndex() == 0, "CaseInvalid GetIndex");
        Test(caseInvalid.ToString() == "PREFIX123", "CaseInvalid ToString");

        const auto caseValid = Strings::PrefixedIndex::ParseNoCase("prefix", "PREFIX123");
        Test(caseValid.IsValid(), "CaseValid IsValid");
        Test(caseValid.GetIndex() == 123, "CaseValid GetIndex");
        Test(caseValid.ToString() == "PREFIX123", "CaseValid ToString");
      }
    }
    std::cout << "---- Test for conversion ----" << std::endl;
    {
      Test(Strings::ConvertFrom(0) == "0", "ConvertFrom zero");
      Test(Strings::ConvertFrom(12345) == "12345", "ConvertFrom positive");
      Test(Strings::ConvertFrom(-23456) == "-23456", "ConvertFrom negative");
      TestParse<int>("zero", "0", 0, "");
      TestParse<uint8_t>("positive overflow", "123456", uint8_t(123456), "");
      TestParse<uint_t>("positive", "123456", 123456, "");
      TestParse<uint64_t>("max positive", "18446744073709551615", 18446744073709551615uLL, "");
      TestParse<int_t>("negative", "-123456", -123456, "");
      TestParse<uint_t>("plus positive", "+123456", 123456, "");
      TestParse<uint_t>("with suffix", "1234M", 1234, "M");
    }
    std::cout << "---- Test for format ----" << std::endl;
    {
      Test(Strings::FormatTime(0, 123, 0, 4) == "123:00.04", "Format time no hours");
      Test(Strings::FormatTime(2, 0, 4, 25) == "2:00:04.25", "Format time");
      Test(Strings::Format("Just string") == "Just string", "No args");
      Test(Strings::Format("Integer: {}", 1234) == "Integer: 1234", "Integer arg");
      Test(Strings::Format("String: '{}'", "str") == "String: 'str'", "String arg");
      Test(Strings::Format("{1} positional {0}", "args", 2) == "2 positional args", "Positional args");
      Test(Strings::Format("Hex {:04x}", 0xbed) == "Hex 0bed", "Formatting");
    }
    std::cout << "---- Test for split ----" << std::endl;
    {
      TestSplit("Empty", "", ' ', {""});
      TestSplit("Single", "single", ',', {"single"});
      TestSplit("Double", "single,,double", ',', {"single", "double"});
      TestSplit("Prefix", ",,str", ',', {"", "str"});
      TestSplit("Suffix", "str,,", ',', {"str", ""});
      TestSplit("MultiDelimiters", ":one,two/three;four.", ";/,.:"_sv, {"", "one", "two", "three", "four", ""});
      TestSplit("Predicate", "a1bc2;d5", [](Char c) { return !std::isalpha(c); }, {"a", "bc", "d", ""});
    }
    std::cout << "---- Test for join ----" << std::endl;
    {
      TestJoin<StringView>("StringView");
      TestJoin<String>("String");
    }
    std::cout << "---- Test for ValueMap ----" << std::endl;
    {
      {
        Strings::ValueMap<String> mapOfStrings{{"key", "value"}};
        TestEquals("value", mapOfStrings["key"], "String.get_key");
        Test(mapOfStrings["key2"].empty() && mapOfStrings.size() == 2, "String.allocate_key");
        TestEquals("", *mapOfStrings.FindPtr("key2"), "String.FindPtr.existing");
        Test(mapOfStrings.Get("key3").empty() && mapOfStrings.size() == 2, "String.get_with_default");
        Test(!mapOfStrings.FindPtr("key3"), "String.FindPtr.nonexisting");
      }
      {
        const auto value = std::make_shared<String>("value");
        Strings::ValueMap<decltype(value)> mapOfPointers{{"key", value}};
        TestEquals(value, mapOfPointers["key"], "Pointer.get_key");
        Test(!mapOfPointers["key2"] && mapOfPointers.size() == 2, "Pointer.allocate_key");
        TestEquals(value.get(), mapOfPointers.FindPtrValue("key"), "Pointer.FindPtrValue.filled");
        Test(!mapOfPointers.Get("key3") && mapOfPointers.size() == 2, "Pointers.get_with_default");
        Test(!mapOfPointers.FindPtrValue("key3"), "Pointer.FindPtr.nonexisting");
      }
    }
    std::cout << "---- Test for casing functions ----" << std::endl;
    {
      TestEquals("TEST", Strings::ToUpperAscii("tEst"), "ToUpper");
      TestEquals("test", Strings::ToLowerAscii("TEsT"), "ToLower");
      Test(Strings::EqualNoCaseAscii("tEst", "TesT"), "EqualNoCase");
      Test(!Strings::EqualNoCaseAscii(" ", ""), "EqualNoCase false");
    }
  }
  catch (int code)
  {
    return code;
  }
}
