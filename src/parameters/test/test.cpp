/**
 *
 * @file
 *
 * @brief  Parameters test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/container_factories.h"
#include "parameters/container.h"
#include "parameters/convert.h"
#include "parameters/identifiers_set.h"
#include "parameters/types.h"
#include "parameters/value.h"

#include "string_view.h"

#include <iostream>
#include <iterator>
#include <type_traits>

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

  void TestIdentifiersSet()
  {
    std::cout << "---- Test for Parameters::IdentifiersSet" << std::endl;
    using namespace Parameters;
    const auto one = "one"_id;
    const auto two = "two"_id;
    const auto three = "three"_id;
    {
      const IdentifiersSet empty;
      Test("empty.ToString()", empty.ToString(), String());
      Test("empty.Contains(one)", empty.Contains(one), false);
    }
    {
      const IdentifiersSet ids{one, two};
      Test("ids.Contains(one)", ids.Contains(one), true);
      Test("ids.Contains(two)", ids.Contains(two), true);
      Test("ids.Contains(three)", ids.Contains(three), false);
      Test("ids.ToString()", ids.ToString(), String("one\ntwo"));
    }
    {
      // duplicates are ignored
      const IdentifiersSet ids{one, one, two};
      Test("dedup.ToString()", ids.ToString(), String("one\ntwo"));
    }
    {
      // empty lines and trailing newline are ignored
      const auto ids = IdentifiersSet::Parse("\none\n\ntwo\n");
      Test("parse.Contains(one)", ids.Contains(one), true);
      Test("parse.Contains(two)", ids.Contains(two), true);
      Test("parse.Contains(three)", ids.Contains(three), false);
      Test("parse.ToString()", ids.ToString(), String("one\ntwo"));
      Test("parse empty", IdentifiersSet::Parse("").ToString(), String());
    }
    {
      // views into the provided storage, which must outlive the set
      const String storage = "one\ntwo";
      const auto ids = IdentifiersSet::Parse(storage);
      Test("external.Contains(one)", ids.Contains(one), true);
      Test("external.Contains(three)", ids.Contains(three), false);
      Test("external.ToString()", ids.ToString(), String("one\ntwo"));
    }
    {
      const IdentifiersSet lhs{one, two};
      const IdentifiersSet rhs{two, three};
      const auto uni = lhs | rhs;
      Test("union.Contains(one)", uni.Contains(one), true);
      Test("union.Contains(two)", uni.Contains(two), true);
      Test("union.Contains(three)", uni.Contains(three), true);
      Test("union.ToString()", uni.ToString(), String("one\ntwo\nthree"));
    }
    {
      // union with a single static identifier
      const auto uni = IdentifiersSet{one} | two;
      Test("set|id.Contains(two)", uni.Contains(two), true);
      Test("set|id.ToString()", uni.ToString(), String("one\ntwo"));
    }
    {
      // union with a single runtime identifier
      const Identifier runtime("two");
      const auto uni = IdentifiersSet{one} | runtime;
      Test("set|runtime.Contains(two)", uni.Contains(two), true);
    }
    {
      // parsed and static sets merge their views
      const auto uni = IdentifiersSet::Parse("one") | IdentifiersSet{two};
      Test("mixed union.Contains(one)", uni.Contains(one), true);
      Test("mixed union.Contains(two)", uni.Contains(two), true);
      Test("mixed union.ToString()", uni.ToString(), String("one\ntwo"));
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

  namespace TestParams
  {
    using namespace Parameters;

    struct INT : Value<uint64_t>
    {
      static constexpr auto Key = "int"_id;
      static constexpr Type Default = 5, Min = 1, Max = 10;
    };

    struct STR : Value<StringType>
    {
      static constexpr auto Key = "str"_id;
      static constexpr const char* Default = "def";
    };

    struct DATA : Value<Binary::Data>
    {
      static constexpr auto Key = "data"_id;
    };

    struct BOOL : Value<bool>
    {
      static constexpr auto Key = "bool"_id;
      static constexpr Type Default = false;
    };

  }  // namespace TestParams

  template<class P>
  concept CanGet = requires(Parameters::Accessor& ac)
  {
    Parameters::Get<P>(ac);
  };

  static_assert(std::is_same_v<Parameters::Value<uint64_t>::Protocol, Parameters::Detail::IntegerProtocol>);
  static_assert(std::is_same_v<Parameters::Value<bool>::Protocol, Parameters::Detail::IntegerProtocol>);
  static_assert(
      std::is_same_v<Parameters::Value<Parameters::StringType>::Protocol, Parameters::Detail::StringProtocol>);
  static_assert(std::is_same_v<Parameters::Value<Binary::Data>::Protocol, Parameters::Detail::DataProtocol>);
  static_assert(Parameters::Defaulted<TestParams::INT>);
  static_assert(Parameters::Defaulted<TestParams::STR>);
  static_assert(Parameters::Defaulted<TestParams::BOOL>);
  static_assert(!Parameters::Defaulted<TestParams::DATA>);
  static_assert(CanGet<TestParams::INT> && CanGet<TestParams::STR> && !CanGet<TestParams::DATA>);
  static_assert(Parameters::Validate<TestParams::INT>(1));
  static_assert(Parameters::Validate<TestParams::INT>(10));
  static_assert(!Parameters::Validate<TestParams::INT>(0));
  static_assert(!Parameters::Validate<TestParams::INT>(11));
  static_assert(Parameters::Validate<TestParams::STR>("x"));

  void TestValue()
  {
    std::cout << "---- Test for Parameters::Value" << std::endl;
    using namespace Parameters;
    using namespace TestParams;

    const auto cont = Container::Create();
    Test("unset int", Get<INT>(*cont), uint64_t(5));
    Test("unset str", Get<STR>(*cont), String("def"));
    Test("unset data", !!Find<DATA>(*cont), false);

    Set<INT>(*cont, IntType(7));
    Test("stored int", *Find<INT>(*cont), uint64_t(7));
    Test("int is not str", !!Find<STR>(*cont), false);

    Set<STR>(*cont, "hello"sv);
    Test("stored str", *Find<STR>(*cont), String("hello"));

    Set<DATA>(*cont, Binary::View("abcd", 4));
    Test("stored data", !!Find<DATA>(*cont), true);

    Test("unset bool", Get<BOOL>(*cont), false);

    Set<BOOL>(*cont, true);
    Test("stored bool true", *Find<BOOL>(*cont), true);
    Test("bool true stored as 1", *cont->FindInteger("bool"_id), IntType(1));

    Set<BOOL>(*cont, false);
    Test("stored bool false", *Find<BOOL>(*cont), false);
    Test("bool false stored as 0", *cont->FindInteger("bool"_id), IntType(0));

    // any non-zero stored value means true
    cont->SetValue("bool"_id, IntType(7));
    Test("non-zero reads true", *Find<BOOL>(*cont), true);
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
    TestValue();
    TestIdentifiersSet();
    TestContainer();
  }
  catch (int code)
  {
    return code;
  }
}
