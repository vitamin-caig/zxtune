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

  void TestNameType()
  {
    std::cout << "---- Test for Parameters::NameType" << std::endl;
    using namespace Parameters;
    const NameType zero{};
    const NameType one("one");
    const NameType two("one.two");
    const NameType three("one.two.three");
    Test("zero.IsEmpty", zero.IsEmpty(), true);
    Test("zero.IsPath", zero.IsPath(), false);
    Test("zero.IsSubpathOf(zero)", zero.IsSubpathOf(zero), false);
    Test("zero.IsSubpathOf(one)", zero.IsSubpathOf(one), false);
    Test("zero.IsSubpathOf(two)", zero.IsSubpathOf(two), false);
    Test("zero.IsSubpathOf(three)", zero.IsSubpathOf(three), false);
    Test("zero + zero", (zero + zero).FullPath(), String());
    Test("zero + one", (zero + one).FullPath(), String("one"));
    Test("zero + two", (zero + two).FullPath(), String("one.two"));
    Test("zero + three", (zero + three).FullPath(), String("one.two.three"));
    Test("zero - zero", (zero - zero).FullPath(), String());
    Test("zero - one", (zero - one).FullPath(), String());
    Test("zero - two", (zero - two).FullPath(), String());
    Test("zero - three", (zero - three).FullPath(), String());
    Test("zero.Name", zero.Name(), String());

    Test("one.IsEmpty", one.IsEmpty(), false);
    Test("one.IsPath", one.IsPath(), false);
    Test("one.IsSubpathOf(zero)", one.IsSubpathOf(zero), false);
    Test("one.IsSubpathOf(one)", one.IsSubpathOf(one), false);
    Test("one.IsSubpathOf(two)", one.IsSubpathOf(two), false);
    Test("one.IsSubpathOf(three)", one.IsSubpathOf(three), false);
    Test("one + zero", (one + zero).FullPath(), String("one"));
    Test("one + one", (one + one).FullPath(), String("one.one"));
    Test("one + two", (one + two).FullPath(), String("one.one.two"));
    Test("one + three", (one + three).FullPath(), String("one.one.two.three"));
    Test("one - zero", (one - zero).FullPath(), String());
    Test("one - one", (one - one).FullPath(), String());
    Test("one - two", (one - two).FullPath(), String());
    Test("one - three", (one - three).FullPath(), String());
    Test("one.Name", one.Name(), String("one"));

    Test("two.IsEmpty", two.IsEmpty(), false);
    Test("two.IsPath", two.IsPath(), true);
    Test("two.IsSubpathOf(zero)", two.IsSubpathOf(zero), false);
    Test("two.IsSubpathOf(one)", two.IsSubpathOf(one), true);
    Test("two.IsSubpathOf(two)", two.IsSubpathOf(two), false);
    Test("two.IsSubpathOf(three)", two.IsSubpathOf(three), false);
    Test("two + zero", (two + zero).FullPath(), String("one.two"));
    Test("two + one", (two + one).FullPath(), String("one.two.one"));
    Test("two + two", (two + two).FullPath(), String("one.two.one.two"));
    Test("two + three", (two + three).FullPath(), String("one.two.one.two.three"));
    Test("two - zero", (two - zero).FullPath(), String());
    Test("two - one", (two - one).FullPath(), String("two"));
    Test("two - two", (two - two).FullPath(), String());
    Test("two - three", (two - three).FullPath(), String());
    Test("two.Name", two.Name(), String("two"));

    Test("three.IsEmpty", three.IsEmpty(), false);
    Test("three.IsPath", three.IsPath(), true);
    Test("three.IsSubpathOf(zero)", three.IsSubpathOf(zero), false);
    Test("three.IsSubpathOf(one)", three.IsSubpathOf(one), true);
    Test("three.IsSubpathOf(two)", three.IsSubpathOf(two), true);
    Test("three.IsSubpathOf(three)", three.IsSubpathOf(three), false);
    Test("three + zero", (three + zero).FullPath(), String("one.two.three"));
    Test("three + one", (three + one).FullPath(), String("one.two.three.one"));
    Test("three + two", (three + two).FullPath(), String("one.two.three.one.two"));
    Test("three + three", (three + three).FullPath(), String("one.two.three.one.two.three"));
    Test("three - zero", (three - zero).FullPath(), String());
    Test("three - one", (three - one).FullPath(), String("two.three"));
    Test("three - two", (three - two).FullPath(), String("three"));
    Test("three - three", (three - three).FullPath(), String());
    Test("three.Name", three.Name(), String("three"));
  }

  class CountingVisitor : public Parameters::Visitor
  {
  public:
    void SetValue(StringView name, Parameters::IntType val)
    {
      Names.emplace_back(name.to_string());
      Integers.push_back(val);
    }

    void SetValue(StringView name, StringView val)
    {
      Names.emplace_back(name.to_string());
      Strings.emplace_back(val.to_string());
    }

    void SetValue(StringView name, Binary::View val)
    {
      Names.emplace_back(name.to_string());
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
    TestNameType();
    TestContainer();
  }
  catch (int code)
  {
    return code;
  }
}
