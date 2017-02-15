/**
*
* @file
*
* @brief  Strings test
*
* @author vitamin.caig@gmail.com
*
**/

#include <strings/encoding.h>
#include <strings/fields.h>
#include <strings/fields_filter.h>
#include <strings/map.h>
#include <strings/template.h>

#include <iostream>

namespace
{
  template<class Policy>
  class FieldsSourceFromMap : public Policy
  {
  public:
    explicit FieldsSourceFromMap(const Strings::Map& map)
      : Map(map)
    {
    }
    
    String GetFieldValue(const String& name) const override
    {
      const auto it = Map.find(name);
      return it == Map.end() ? Policy::GetFieldValue(name) : it->second;
    }
  private:
    const Strings::Map& Map;
  };
  
  void TestTemplate(const Strings::FieldsSource& source, const String& templ, const String& reference)
  {
    const String res = Strings::Template::Instantiate(templ, source);
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

  template<class Policy>
  void TestTemplate(const String& templ, const Strings::Map& params, const String& reference)
  {
    const FieldsSourceFromMap<Policy> source(params);
    TestTemplate(source, templ, reference);
  }
  
  void TestTranscode(const char* encoding, const String& str, const String& reference)
  {
    const String& trans = Strings::ToAutoUtf8(str);
    if (trans == reference)
    {
      std::cout << "Passed " << encoding << " test '" << str << "' => '" << reference << '\'' << std::endl;
    }
    else
    {
      std::cout << "Failed " << encoding << " test '" << str << "' => '" << reference << "' (result is '" << trans << "')" << std::endl;
    }
  }
}

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
      TestTemplate<Strings::SkipFieldsSource>("duplicate [name] and [name] test", params, "duplicate value and value test");
      params["value"] = "name";
      TestTemplate<Strings::SkipFieldsSource>("multiple [name] and [value] test", params, "multiple value and name test");
      TestTemplate<Strings::SkipFieldsSource>("syntax error [name test", params, "syntax error [name test");
      TestTemplate<Strings::SkipFieldsSource>("[name] at the beginning", params, "value at the beginning");
      TestTemplate<Strings::SkipFieldsSource>("at the end [name]", params, "at the end value");
      const FieldsSourceFromMap<Strings::SkipFieldsSource> source(params);
      const Strings::FilterFieldsSource replaceToChar(source, "abcde", '%');
      TestTemplate(replaceToChar, "Replace bunch of symbols to single in [name] and [value]", "Replace bunch of symbols to single in v%lu% and n%m%");
      const Strings::FilterFieldsSource replaceToCharsSet(source, "abcde", "ABCDE");
      TestTemplate(replaceToCharsSet, "Replace bunch of symbols to multiple in [name] and [value]", "Replace bunch of symbols to multiple in vAluE and nAmE");
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
      TestTranscode("CP1251", "\xe4\xe5\xe4\xf3\xf8\xea\xe0", "\xd0\xb4\xd0\xb5\xd0\xb4\xd1\x83\xd1\x88\xd0\xba\xd0\xb0");
      TestTranscode("SJIS", "\x83\x50\x83\x43\x83\x93\x82\xcc\x83\x65\x81\x5b\x83\x7d", "\xe3\x82\xb1\xe3\x82\xa4\xe3\x83\xb3\xe3\x81\xae\xe3\x83\x86\xe3\x83\xbc\xe3\x83\x9e");
    }
  }
  catch (int code)
  {
    return code;
  }
}
