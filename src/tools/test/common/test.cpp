#include <byteorder.h>
#include <iterator.h>
#include <parameters.h>
#include <range_checker.h>
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

  template<class T>
  void TestScale(const std::string& test, T val, T inScale, T outScale, T result)
  {
    Test<T>("Scale " + test, Scale(val, inScale, outScale), result);
    const ScaleFunctor<T> scaler(inScale, outScale);
    Test<T>("ScaleFunctor " + test, scaler(val), result);
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
  std::cout << "---- Test for Scale -----" << std::endl;
  {
    Test<uint8_t>("Log2(uint8_t)", Log2(uint8_t(0xff)), 8);
    Test<uint16_t>("Log2(uint16_t)", Log2(uint16_t(0xffff)), 16);
    Test<uint32_t>("Log2(uint32_t)", Log2(uint32_t(0xffffffff)), 32);
    Test<uint64_t>("Log2(uint64_t)", Log2(UINT64_C(0xffffffffffffffff)), 64);
    TestScale<uint32_t>("uint32_t small up", 1000, 3000, 2000, 666);
    TestScale<uint32_t>("uint32_t small up overhead", 4000, 3000, 2000, 2666);
    TestScale<uint32_t>("uint32_t small down", 1000, 2000, 3000, 1500);
    TestScale<uint32_t>("uint32_t small down overhead", 4000, 2000, 3000, 6000);
    TestScale<uint32_t>("uint32_t medium up", 10000, 30000, 20000, 6666);
    TestScale<uint32_t>("uint32_t medium up overhead", 40000, 30000, 20000, 26666);
    TestScale<uint32_t>("uint32_t medium down", 10000, 20000, 30000, 15000);
    TestScale<uint32_t>("uint32_t medium down overhead", 40000, 20000, 30000, 60000);
    TestScale<uint32_t>("uint32_t big up", 1000000, 3000000, 2000000, 666666);
    TestScale<uint32_t>("uint32_t big up overhead", 4000000, 3000000, 2000000, 2666666);

    TestScale<uint64_t>("uint64_t small up", 1000, 3000, 2000, 666);
    TestScale<uint64_t>("uint64_t small up overhead", 4000, 3000, 2000, 2666);
    TestScale<uint64_t>("uint64_t small down", 1000, 2000, 3000, 1500);
    TestScale<uint64_t>("uint64_t small down overhead", 4000, 2000, 3000, 6000);
    TestScale<uint64_t>("uint64_t medium up", 10000, 30000, 20000, 6666);
    TestScale<uint64_t>("uint64_t medium up overhead", 40000, 30000, 20000, 26666);
    TestScale<uint64_t>("uint64_t medium down", 10000, 20000, 30000, 15000);
    TestScale<uint64_t>("uint64_t medium down overhead", 40000, 20000, 30000, 60000);
    TestScale<uint64_t>("uint64_t big up", 1000000, 3000000, 2000000, 666666);
    TestScale<uint64_t>("uint64_t big up overhead", 4000000, 3000000, 2000000, 2666666);
    TestScale<uint64_t>("uint64_t big down", 1000000, 2000000, 3000000, 1500000);
    TestScale<uint64_t>("uint64_t big down overhead", 4000000, 2000000, 3000000, 6000000);
    TestScale<uint64_t>("uint64_t giant up", UINT64_C(10000000000), UINT64_C(30000000000), UINT64_C(20000000000), UINT64_C(6666666666));
    TestScale<uint64_t>("uint64_t giant up overhead", UINT64_C(4000000000), UINT64_C(3000000000), UINT64_C(20000000000), UINT64_C(26666666666));
    TestScale<uint64_t>("uint64_t giant down", UINT64_C(10000000000), UINT64_C(20000000000), UINT64_C(30000000000), UINT64_C(15000000000));
    TestScale<uint64_t>("uint64_t giant down overhead", UINT64_C(4000000000), UINT64_C(2000000000), UINT64_C(3000000000), UINT64_C(6000000000));
  }
  std::cout << "---- Test for iterators ----" << std::endl;
  {
    const uint8_t DATA[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CycledIterator<const uint8_t*> it(DATA, ArrayEnd(DATA));
    Test<uint_t>("Cycled iterator init", *it, 0);
    Test<uint_t>("Cycled iterator forward", *++it, 1);
    Test<uint_t>("Cycled iterator backward", *--it, 0);
    Test<uint_t>("Cycled iterator underrun", *--it, 9);
    Test<uint_t>("Cycled iterator overrun", *++it, 0);
    Test<uint_t>("Cycled iterator positive delta", *(it + 15), 5);
    Test<uint_t>("Cycled iterator negative delta", *(it - 11), 9);
  }
  std::cout << "---- Test for Parameters::NameType" << std::endl;
  {
    using namespace Parameters;
    const NameType zero;
    const NameType one("one");
    const NameType two("one.two");
    const NameType three("one.two.three");
    Test("zero.IsEmpty", zero.IsEmpty(), true);
    Test("zero.IsPath", zero.IsPath(), false);
    Test("zero.IsSubpathOf(zero)", zero.IsSubpathOf(zero), false);
    Test("zero.IsSubpathOf(one)", zero.IsSubpathOf(one), false);
    Test("zero.IsSubpathOf(two)", zero.IsSubpathOf(two), false);
    Test("zero.IsSubpathOf(three)", zero.IsSubpathOf(three), false);
    Test("zero + zero", (zero + zero).FullPath(), std::string());
    Test("zero + one", (zero + one).FullPath(), std::string("one"));
    Test("zero + two", (zero + two).FullPath(), std::string("one.two"));
    Test("zero + three", (zero + three).FullPath(), std::string("one.two.three"));
    Test("zero - zero", (zero - zero).FullPath(), std::string());
    Test("zero - one", (zero - one).FullPath(), std::string());
    Test("zero - two", (zero - two).FullPath(), std::string());
    Test("zero - three", (zero - three).FullPath(), std::string());
    Test("zero.Name", zero.Name(), std::string());

    Test("one.IsEmpty", one.IsEmpty(), false);
    Test("one.IsPath", one.IsPath(), false);
    Test("one.IsSubpathOf(zero)", one.IsSubpathOf(zero), false);
    Test("one.IsSubpathOf(one)", one.IsSubpathOf(one), false);
    Test("one.IsSubpathOf(two)", one.IsSubpathOf(two), false);
    Test("one.IsSubpathOf(three)", one.IsSubpathOf(three), false);
    Test("one + zero", (one + zero).FullPath(), std::string("one"));
    Test("one + one", (one + one).FullPath(), std::string("one.one"));
    Test("one + two", (one + two).FullPath(), std::string("one.one.two"));
    Test("one + three", (one + three).FullPath(), std::string("one.one.two.three"));
    Test("one - zero", (one - zero).FullPath(), std::string());
    Test("one - one", (one - one).FullPath(), std::string());
    Test("one - two", (one - two).FullPath(), std::string());
    Test("one - three", (one - three).FullPath(), std::string());
    Test("one.Name", one.Name(), std::string("one"));

    Test("two.IsEmpty", two.IsEmpty(), false);
    Test("two.IsPath", two.IsPath(), true);
    Test("two.IsSubpathOf(zero)", two.IsSubpathOf(zero), false);
    Test("two.IsSubpathOf(one)", two.IsSubpathOf(one), true);
    Test("two.IsSubpathOf(two)", two.IsSubpathOf(two), false);
    Test("two.IsSubpathOf(three)", two.IsSubpathOf(three), false);
    Test("two + zero", (two + zero).FullPath(), std::string("one.two"));
    Test("two + one", (two + one).FullPath(), std::string("one.two.one"));
    Test("two + two", (two + two).FullPath(), std::string("one.two.one.two"));
    Test("two + three", (two + three).FullPath(), std::string("one.two.one.two.three"));
    Test("two - zero", (two - zero).FullPath(), std::string());
    Test("two - one", (two - one).FullPath(), std::string("two"));
    Test("two - two", (two - two).FullPath(), std::string());
    Test("two - three", (two - three).FullPath(), std::string());
    Test("two.Name", two.Name(), std::string("two"));

    Test("three.IsEmpty", three.IsEmpty(), false);
    Test("three.IsPath", three.IsPath(), true);
    Test("three.IsSubpathOf(zero)", three.IsSubpathOf(zero), false);
    Test("three.IsSubpathOf(one)", three.IsSubpathOf(one), true);
    Test("three.IsSubpathOf(two)", three.IsSubpathOf(two), true);
    Test("three.IsSubpathOf(three)", three.IsSubpathOf(three), false);
    Test("three + zero", (three + zero).FullPath(), std::string("one.two.three"));
    Test("three + one", (three + one).FullPath(), std::string("one.two.three.one"));
    Test("three + two", (three + two).FullPath(), std::string("one.two.three.one.two"));
    Test("three + three", (three + three).FullPath(), std::string("one.two.three.one.two.three"));
    Test("three - zero", (three - zero).FullPath(), std::string());
    Test("three - one", (three - one).FullPath(), std::string("two.three"));
    Test("three - two", (three - two).FullPath(), std::string("three"));
    Test("three - three", (three - three).FullPath(), std::string());
    Test("three.Name", three.Name(), std::string("three"));
  }
  }
  catch (int code)
  {
    return code;
  }
}
