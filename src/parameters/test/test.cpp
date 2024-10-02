/**
 *
 * @file
 *
 * @brief  Parameters test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/container_factories.h>
#include <parameters/container.h>
#include <parameters/convert.h>
#include <parameters/types.h>

#include <iostream>
#include <iterator>

template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
  s << '[';
  std::copy(v.begin(), v.end(), std::ostream_iterator<T>(s, ","));
  s << ']';
  return s;
}

namespace
{
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

  void Test(const String& msg, Parameters::Identifier id, StringView ref)
  {
    Test<StringView>(msg, id, ref);
  }

  void TestIdentifier()
  {
    std::cout << "---- Test for Parameters::StaticIdentifier" << std::endl;
    using namespace Parameters;
    const auto zero = ""_id;
    const auto one = "one"_id;
    const auto two = "one.two"_id;
    const auto three = "one.two.three"_id;
    {
      const Identifier zeroId = zero;
      Test("zero.IsEmpty", zeroId.IsEmpty(), true);
      Test("zero.IsPath", zeroId.IsPath(), false);
      Test("zero.Name()", zeroId.Name(), ""sv);
      Test("zero.RelativeTo(zero)", zeroId.RelativeTo(zero), ""sv);
      Test("zero.RelativeTo(one)", zeroId.RelativeTo(one), ""sv);
      Test("zero.RelativeTo(two)", zeroId.RelativeTo(two), ""sv);
      Test("zero.RelativeTo(three)", zeroId.RelativeTo(three), ""sv);
      Test("zero.Append(one)", zeroId.Append(one), "one"sv);
    }

    {
      const Identifier oneId = one;
      Test("one.IsEmpty", oneId.IsEmpty(), false);
      Test("one.IsPath", oneId.IsPath(), false);
      Test("one.Name()", oneId.Name(), "one"sv);
      Test("one.RelativeTo(zero)", oneId.RelativeTo(zero), ""sv);
      Test("one.RelativeTo(one)", oneId.RelativeTo(one), ""sv);
      Test("one.RelativeTo(two)", oneId.RelativeTo(two), ""sv);
      Test("one.RelativeTo(three)", oneId.RelativeTo(three), ""sv);
      Test("one + one", one + one, "one.one"sv);
      Test("one + two", one + two, "one.one.two"sv);
      Test("one + three", one + three, "one.one.two.three"sv);
      Test("one.Append(zero)", oneId.Append(zero), "one"sv);
    }

    {
      const Identifier twoId = two;
      Test("two.IsEmpty", twoId.IsEmpty(), false);
      Test("two.IsPath", twoId.IsPath(), true);
      Test("two.Name()", twoId.Name(), "two"sv);
      Test("two.RelativeTo(zero)", twoId.RelativeTo(zero), ""sv);
      Test("two.RelativeTo(one)", twoId.RelativeTo(one), "two"sv);
      Test("two.RelativeTo(two)", twoId.RelativeTo(two), ""sv);
      Test("two.RelativeTo(three)", twoId.RelativeTo(three), ""sv);
      Test("two + one", two + one, "one.two.one"sv);
      Test("two + two", two + two, "one.two.one.two"sv);
      Test("two + three", two + three, "one.two.one.two.three"sv);
    }

    {
      const Identifier threeId = three;
      Test("three.IsEmpty", threeId.IsEmpty(), false);
      Test("three.IsPath", threeId.IsPath(), true);
      Test("three.Name()", threeId.Name(), "three"sv);
      Test("three.RelativeTo(zero)", threeId.RelativeTo(zero), ""sv);
      Test("three.IsSubpathOf(one)", threeId.RelativeTo(one), "two.three"sv);
      Test("three.IsSubpathOf(two)", threeId.RelativeTo(two), "three"sv);
      Test("three.IsSubpathOf(three)", threeId.RelativeTo(three), ""sv);
      Test("three + one", three + one, "one.two.three.one"sv);
      Test("three + two", three + two, "one.two.three.one.two"sv);
      Test("three + three", three + three, "one.two.three.one.two.three"sv);
    }
  }

  class CountingVisitor : public Parameters::Visitor
  {
  public:
    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      Names.emplace_back(name.AsString());
      Integers.push_back(val);
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      Names.emplace_back(name.AsString());
      Strings.emplace_back(val);
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      Names.emplace_back(name.AsString());
      Datas.emplace_back(Binary::CreateContainer(val));
    }

    std::vector<String> Names;
    std::vector<Parameters::IntType> Integers;
    std::vector<Parameters::StringType> Strings;
    std::vector<Binary::Data::Ptr> Datas;
  };

  void TestFind(const Parameters::Accessor& src, StringView name, const Parameters::IntType* ref)
  {
    const auto out = src.FindInteger(name);
    Test("find", !!out, !!ref);
    if (ref != nullptr)
    {
      Test("value", *out, *ref);
    }
  }

  void TestFind(const Parameters::Accessor& src, StringView name, const Parameters::StringType* ref)
  {
    const auto out = src.FindString(name);
    Test("find", !!out, !!ref);
    if (ref != nullptr)
    {
      Test("value", *out, *ref);
    }
  }

  void TestFind(const Parameters::Accessor& src, StringView name, const Binary::Dump* ref)
  {
    const auto out = src.FindData(name);
    Test("find", !!out, !!ref);
    if (ref)
    {
      Test("value", Parameters::ConvertToString(*out), Parameters::ConvertToString(*ref));
    }
  }

  void TestContainer()
  {
    std::cout << "---- Test for Parameters::Container::Create" << std::endl;
    const auto cont = Parameters::Container::Create();
    std::cout << "Initial" << std::endl;
    Test("version", cont->Version(), 0u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("count", cnt.Integers.size() + cnt.Strings.size() + cnt.Datas.size(), std::size_t(0));
    }

    const Parameters::IntType int1 = 1;
    const Parameters::IntType int2 = 2;
    const Parameters::IntType int3 = 3;
    const Parameters::IntType int4 = 4;
    const Parameters::IntType* noInt = nullptr;
    const Parameters::StringType str1 = "a";
    const Parameters::StringType str2 = "b";
    const Parameters::StringType str3 = "c";
    const Parameters::StringType str4 = "d";
    const Parameters::StringType* noString = nullptr;
    const Binary::Dump data = {};
    const Binary::Dump* noData = nullptr;

    std::cout << "3 inseters" << std::endl;
    cont->SetValue("int", int1);
    cont->SetValue("str", str1);
    cont->SetValue("bin", data);
    Test(" version", cont->Version(), 3u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test(" count", cnt.Integers.size() * cnt.Strings.size() * cnt.Datas.size(), std::size_t(1));
    }

    std::cout << "3 updates with type change" << std::endl;
    cont->SetValue("str", int2);
    cont->SetValue("bin", data);
    cont->SetValue("int", str2);
    Test("version", cont->Version(), 6u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("names", cnt.Names, {"str", "int", "bin"});
      Test("integers", cnt.Integers, {int2});
      Test("strings", cnt.Strings, {str2});
      Test("datas count", cnt.Datas.size(), std::size_t(1));
    }

    std::cout << "3 updates, 2 ignored" << std::endl;
    cont->SetValue("str", int2);
    cont->SetValue("bin", data);
    cont->SetValue("int", str2);
    Test("version", cont->Version(), 7u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("count", cnt.Integers.size() * cnt.Strings.size() * cnt.Datas.size(), std::size_t(1));
      Test("names", cnt.Names, {"str", "int", "bin"});
      Test("integers", cnt.Integers, {int2});
      Test("strings", cnt.Strings, {str2});
      Test("datas count", cnt.Datas.size(), std::size_t(1));
    }

    std::cout << "2 updates" << std::endl;
    cont->SetValue("str", int3);
    cont->SetValue("int", str3);
    Test("version", cont->Version(), 9u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("count", cnt.Integers.size() * cnt.Strings.size() * cnt.Datas.size(), std::size_t(1));
      Test("names", cnt.Names, {"str", "int", "bin"});
      Test("integers", cnt.Integers, {int3});
      Test("strings", cnt.Strings, {str3});
      Test("datas count", cnt.Datas.size(), std::size_t(1));
    }

    std::cout << "3 addings" << std::endl;
    cont->SetValue("str2int", int4);
    cont->SetValue("int2str", str4);
    Test("version", cont->Version(), 11u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("names", cnt.Names, {"str", "str2int", "int", "int2str", "bin"});
      Test("integers", cnt.Integers, {int3, int4});
      Test("strings", cnt.Strings, {str3, str4});
      Test("datas count", cnt.Datas.size(), std::size_t(1));

      TestFind(*cont, "str", &int3);
      TestFind(*cont, "str", noString);
      TestFind(*cont, "str", noData);
      TestFind(*cont, "str2int", &int4);
      TestFind(*cont, "str2int", noString);
      TestFind(*cont, "str2int", noData);
      TestFind(*cont, "int", &str3);
      TestFind(*cont, "int", noInt);
      TestFind(*cont, "int", noData);
      TestFind(*cont, "int2str", &str4);
      TestFind(*cont, "int2str", noInt);
      TestFind(*cont, "int2str", noData);
      TestFind(*cont, "bin", noString);
      TestFind(*cont, "bin", noInt);
      TestFind(*cont, "bin", &data);
    }

    std::cout << "ignored erase" << std::endl;
    cont->RemoveValue("ignored");
    Test("version", cont->Version(), 11u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("names", cnt.Names, {"str", "str2int", "int", "int2str", "bin"});
      Test("integers", cnt.Integers, {int3, int4});
      Test("strings", cnt.Strings, {str3, str4});
      Test("datas count", cnt.Datas.size(), std::size_t(1));
    }

    std::cout << "accounted erases" << std::endl;
    cont->RemoveValue("str");
    cont->RemoveValue("int");
    cont->RemoveValue("bin");
    Test("version", cont->Version(), 14u);
    {
      CountingVisitor cnt;
      cont->Process(cnt);
      Test("names", cnt.Names, {"str2int", "int2str"});
      Test("integers", cnt.Integers, {int4});
      Test("strings", cnt.Strings, {str4});

      TestFind(*cont, "str", noInt);
      TestFind(*cont, "str", noString);
      TestFind(*cont, "str", noData);
      TestFind(*cont, "str2int", &int4);
      TestFind(*cont, "str2int", noString);
      TestFind(*cont, "str2int", noData);
      TestFind(*cont, "int", noString);
      TestFind(*cont, "int", noInt);
      TestFind(*cont, "int", noData);
      TestFind(*cont, "int2str", &str4);
      TestFind(*cont, "int2str", noInt);
      TestFind(*cont, "int2str", noData);
      TestFind(*cont, "bin", noString);
      TestFind(*cont, "bin", noInt);
      TestFind(*cont, "bin", noData);
    }
    Test("final version", cont->Version(), 14u);
  }
}  // namespace

int main()
{
  try
  {
    TestIdentifier();
    TestContainer();
  }
  catch (int code)
  {
    return code;
  }
}
