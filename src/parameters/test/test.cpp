/**
 *
 * @file
 *
 * @brief  Parameters test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <parameters/container.h>
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
      Test("zero.Name()", zeroId.Name(), ""_sv);
      Test("zero.RelativeTo(zero)", zeroId.RelativeTo(zero), ""_sv);
      Test("zero.RelativeTo(one)", zeroId.RelativeTo(one), ""_sv);
      Test("zero.RelativeTo(two)", zeroId.RelativeTo(two), ""_sv);
      Test("zero.RelativeTo(three)", zeroId.RelativeTo(three), ""_sv);
      Test("zero.Append(one)", zeroId.Append(one), "one"_sv);
    }

    {
      const Identifier oneId = one;
      Test("one.IsEmpty", oneId.IsEmpty(), false);
      Test("one.IsPath", oneId.IsPath(), false);
      Test("one.Name()", oneId.Name(), "one"_sv);
      Test("one.RelativeTo(zero)", oneId.RelativeTo(zero), ""_sv);
      Test("one.RelativeTo(one)", oneId.RelativeTo(one), ""_sv);
      Test("one.RelativeTo(two)", oneId.RelativeTo(two), ""_sv);
      Test("one.RelativeTo(three)", oneId.RelativeTo(three), ""_sv);
      Test("one + one", one + one, "one.one"_sv);
      Test("one + two", one + two, "one.one.two"_sv);
      Test("one + three", one + three, "one.one.two.three"_sv);
      Test("one.Append(zero)", oneId.Append(zero), "one"_sv);
    }

    {
      const Identifier twoId = two;
      Test("two.IsEmpty", twoId.IsEmpty(), false);
      Test("two.IsPath", twoId.IsPath(), true);
      Test("two.Name()", twoId.Name(), "two"_sv);
      Test("two.RelativeTo(zero)", twoId.RelativeTo(zero), ""_sv);
      Test("two.RelativeTo(one)", twoId.RelativeTo(one), "two"_sv);
      Test("two.RelativeTo(two)", twoId.RelativeTo(two), ""_sv);
      Test("two.RelativeTo(three)", twoId.RelativeTo(three), ""_sv);
      Test("two + one", two + one, "one.two.one"_sv);
      Test("two + two", two + two, "one.two.one.two"_sv);
      Test("two + three", two + three, "one.two.one.two.three"_sv);
    }

    {
      const Identifier threeId = three;
      Test("three.IsEmpty", threeId.IsEmpty(), false);
      Test("three.IsPath", threeId.IsPath(), true);
      Test("three.Name()", threeId.Name(), "three"_sv);
      Test("three.RelativeTo(zero)", threeId.RelativeTo(zero), ""_sv);
      Test("three.IsSubpathOf(one)", threeId.RelativeTo(one), "two.three"_sv);
      Test("three.IsSubpathOf(two)", threeId.RelativeTo(two), "three"_sv);
      Test("three.IsSubpathOf(three)", threeId.RelativeTo(three), ""_sv);
      Test("three + one", three + one, "one.two.three.one"_sv);
      Test("three + two", three + two, "one.two.three.one.two"_sv);
      Test("three + three", three + three, "one.two.three.one.two.three"_sv);
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
      Strings.emplace_back(val.to_string());
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      Names.emplace_back(name.AsString());
      const auto* raw = val.As<uint8_t>();
      Datas.emplace_back(Binary::Dump(raw, raw + val.Size()));
    }

    std::vector<String> Names;
    std::vector<Parameters::IntType> Integers;
    std::vector<Parameters::StringType> Strings;
    std::vector<Parameters::DataType> Datas;
  };

  template<class T>
  void TestFind(const Parameters::Accessor& src, StringView name, const T* ref)
  {
    T out;
    Test("find", src.FindValue(name, out), ref != nullptr);
    if (ref != nullptr)
    {
      Test("value", out, *ref);
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

    const Parameters::IntType int1 = 1, int2 = 2, int3 = 3, int4 = 4, *noInt = nullptr;
    const Parameters::StringType str1 = "a", str2 = "b", str3 = "c", str4 = "d", *noString = nullptr;
    const Parameters::DataType data = {}, *noData = nullptr;

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
