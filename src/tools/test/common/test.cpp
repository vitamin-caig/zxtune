/**
 *
 * @file
 *
 * @brief  Common tools test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <tools/iterators.h>
#include <tools/range_checker.h>

#include <byteorder.h>
#include <string_type.h>

#include <iostream>

namespace
{
  template<class T>
  void TestType(const char* name)
  {
    const T var = 0;
    std::cout << "sizeof(" << name << " is " << typeid(var).name() << ")=" << sizeof(var) << std::endl;
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

  template<class T>
  void TestOrder(T orig, T test)
  {
    const std::string type = typeid(orig).name();
    const T result = swapBytes(orig);
    Test(type + " swap", result, test);
    uint8_t buf[sizeof(T)];
    std::memcpy(buf, &orig, sizeof(orig));
    if (isLE())
    {
      Test(type + " LE", ReadLE<T>(buf), orig);
      Test(type + " BE", ReadBE<T>(buf), result);
    }
    else
    {
      Test(type + " LE", ReadLE<T>(buf), result);
      Test(type + " BE", ReadBE<T>(buf), orig);
    }

    WriteLE(orig, buf);
    Test(type + " LE -> LE", ReadLE<T>(buf), orig);
    Test(type + " LE -> BE", ReadBE<T>(buf), result);
    WriteBE(orig, buf);
    Test(type + " BE -> LE", ReadLE<T>(buf), result);
    Test(type + " BE -> BE", ReadBE<T>(buf), orig);

    Test<T>(type + " BE<>(0)", BE<T>(), 0);
    Test<T>(type + " BE<>()", BE<T>(orig), orig);

    Test<T>(type + " LE<>(0)", LE<T>(), 0);
    Test<T>(type + " LE<>()", LE<T>(orig), orig);

    union {
      LE<T> le;
      BE<T> be;
    } united;
    std::memcpy(&united, &orig, sizeof(orig));
    if (isLE())
    {
      Test<T>(type + " LE<>", united.le, orig);
      Test<T>(type + " BE<>", united.be, result);
    }
    else
    {
      Test<T>(type + " LE<>", united.le, result);
      Test<T>(type + " BE<>", united.be, orig);
    }
  }

  void Test(const String& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
    {
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

  void TestAreaSizes(const std::string& test, const AreaController& area, std::size_t bsize, std::size_t s1size,
                     std::size_t s2size, std::size_t esize)
  {
    Test(test + ": size of begin", area.GetAreaSize(BEGIN), bsize);
    Test(test + ": size of sect1", area.GetAreaSize(SECTION1), s1size);
    Test(test + ": size of sect2", area.GetAreaSize(SECTION2), s2size);
    Test(test + ": size of end", area.GetAreaSize(END), esize);
    Test(test + ": size of unspec", area.GetAreaSize(UNSPECIFIED), std::size_t(0));
  }
}  // namespace

int main()
{
  try
  {
    TestType<int_t>("int_t");
    TestType<int8_t>("int8_t");
    TestType<int16_t>("int16_t");
    TestType<int32_t>("int32_t");
    TestType<int64_t>("int64_t");
    TestType<char>("char");
    TestType<signed char>("signed char");
    TestType<short>("short");
    TestType<int>("int");
    TestType<long>("long");
    TestType<uint_t>("uint_t");
    TestType<uint8_t>("uint8_t");
    TestType<uint16_t>("uint16_t");
    TestType<uint32_t>("uint32_t");
    TestType<uint64_t>("uint64_t");
    TestType<unsigned char>("unsigned char");
    TestType<unsigned short>("unsigned short");
    TestType<unsigned int>("unsigned int");
    TestType<unsigned long>("unsigned long");
    std::cout << "---- Test for byteorder working ----" << std::endl;
    TestOrder<int16_t>(-30875, 0x6587);
    TestOrder<uint16_t>(0x1234, 0x3412);
    // 0x87654321
    TestOrder<int32_t>(-2023406815, 0x21436587);
    TestOrder<uint32_t>(0x12345678, 0x78563412);
    // 0xfedcab9876543210
    TestOrder<int64_t>(-INT64_C(81985529216486896), UINT64_C(0x1032547698badcfe));
    TestOrder<uint64_t>(UINT64_C(0x123456789abcdef0), UINT64_C(0xf0debc9a78563412));
    {
      union {
        le_uint24_t le;
        be_uint24_t be;
      } value;
      static_assert(sizeof(value) == 3, "Invalid layout");
      value.le = 0x123456;
      Test<uint_t>("24 bit LE->LE", value.le, 0x123456);
      Test<uint_t>("24 bit LE->BE", value.be, 0x563412);
      value.be = 0x234567;
      Test<uint_t>("24 bit BE->BE", value.be, 0x234567);
      Test<uint_t>("24 bit BE->LE", value.le, 0x674523);
      // overflow
      value.le = 0x2345678;
      Test<uint_t>("24 bit LE->LE overflow", value.le, 0x345678);
      Test<uint_t>("24 bit LE->BE overflow", value.be, 0x785634);
      value.be = 0x3456789;
      Test<uint_t>("24 bit BE->BE overflow", value.be, 0x456789);
      Test<uint_t>("24 bit BE->LE overflow", value.le, 0x896745);
    }

    std::cout << "---- Test for ranges map ----" << std::endl;
    {
      std::cout << "  test for simple range checker" << std::endl;
      RangeChecker::Ptr checker(RangeChecker::Create(100));
      Test("Adding too big range", !checker->AddRange(0, 110));
      Test("Adding range to begin", checker->AddRange(30, 10));   // 30..40
      Test("Adding range to lefter", checker->AddRange(20, 10));  //=>20..40
      Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 40));
      Test("Adding range to end", checker->AddRange(70, 10));      // 70..80
      Test("Adding range to righter", checker->AddRange(80, 10));  //=>70..90
      Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
      Test("Adding wrong range from left", !checker->AddRange(30, 30));
      Test("Adding wrong range to right", !checker->AddRange(50, 30));
      Test("Adding wrong range to both", !checker->AddRange(30, 50));
      Test("Adding merged to left", checker->AddRange(40, 10));   // 40..50 => 20..50
      Test("Adding merged to right", checker->AddRange(60, 10));  // 60..70 => 6..90
      Test("Adding merged to mid", checker->AddRange(50, 10));
      Test("Adding zero-sized to free", checker->AddRange(0, 0));
      Test("Adding zero-sized to busy", !checker->AddRange(30, 0));
      Test("Check result", checker->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 90));
      std::cout << "  test for shared range checker" << std::endl;
      RangeChecker::Ptr shared(RangeChecker::CreateShared(100));
      Test("Adding too big range", !shared->AddRange(0, 110));
      Test("Adding zero-sized to free", shared->AddRange(20, 0));
      Test("Adding range to begin", shared->AddRange(20, 20));  // 20..40
      Test("Check result", shared->GetAffectedRange() == std::pair<std::size_t, std::size_t>(20, 40));
      Test("Adding range to end", shared->AddRange(70, 20));  // 70..90
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
        AreaController areas;
        areas.AddArea(BEGIN, 0);
        areas.AddArea(SECTION1, 10);
        areas.AddArea(SECTION2, 20);
        areas.AddArea(END, 30);
        TestAreaSizes("Ordered", areas, 10, 10, 10, AreaController::Undefined);
      }
      {
        AreaController areas;
        areas.AddArea(BEGIN, 40);
        areas.AddArea(SECTION1, 30);
        areas.AddArea(SECTION2, 20);
        areas.AddArea(END, 10);
        TestAreaSizes("Reversed", areas, AreaController::Undefined, 10, 10, 10);
      }
      {
        AreaController areas;
        areas.AddArea(BEGIN, 0);
        areas.AddArea(SECTION1, 30);
        areas.AddArea(SECTION2, 30);
        areas.AddArea(END, 10);
        TestAreaSizes("Duplicated", areas, 10, 0, 0, 20);
      }
    }
    std::cout << "---- Test for iterators ----" << std::endl;
    {
      const uint8_t DATA[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      CycledIterator<const uint8_t*> it(DATA, DATA + 10);
      Test<uint_t>("Cycled iterator init", *it, 0);
      Test<uint_t>("Cycled iterator forward", *++it, 1);
      Test<uint_t>("Cycled iterator backward", *--it, 0);
      Test<uint_t>("Cycled iterator underrun", *--it, 9);
      Test<uint_t>("Cycled iterator overrun", *++it, 0);
      Test<uint_t>("Cycled iterator positive delta", *(it + 15), 5);
      Test<uint_t>("Cycled iterator negative delta", *(it - 11), 9);
    }
  }
  catch (int code)
  {
    return code;
  }
}
