/**
 *
 * @file
 *
 * @brief  Math library test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <iostream>
#include <math/scale.h>

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

#define ConstexprTest(msg, result, reference) static_assert(result == reference, msg)

  template<class T>
  void TestLog2(const std::string& type)
  {
    Test<T>("Log2(" + type + ", zero)", Math::Log2<T>(0), 1);
    Test<T>("Log2(" + type + ", one)", Math::Log2<T>(1), 1);
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    Test<T>("Log2(" + type + ", low half)", Math::Log2<T>(lowHalf), 4 * sizeof(T));
    Test<T>("Log2(" + type + ", high half)", Math::Log2<T>(highHalf), 8 * sizeof(T));
    Test<T>("Log2(" + type + ", max)", Math::Log2<T>(maximal), 8 * sizeof(T));
  }

  template<class T>
  constexpr void ConstexprTestLog2()
  {
    ConstexprTest("Log2(zero)", Math::Log2<T>(0), 1);
    ConstexprTest("Log2(one)", Math::Log2<T>(1), 1);
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    ConstexprTest("Log2(low half)", Math::Log2<T>(lowHalf), 4 * sizeof(T));
    ConstexprTest("Log2(high half)", Math::Log2<T>(highHalf), 8 * sizeof(T));
    ConstexprTest("Log2(max)", Math::Log2<T>(maximal), 8 * sizeof(T));
  }

  template<class T>
  void TestCountBits(const std::string& type)
  {
    Test<T>("CountBits(" + type + ", zero)", Math::CountBits<T>(0), 0);
    Test<T>("CountBits(" + type + ", one)", Math::CountBits<T>(1), 1);
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    Test<T>("CountBits(" + type + ", low half)", Math::CountBits<T>(lowHalf), 4 * sizeof(T));
    Test<T>("CountBits(" + type + ", high half)", Math::CountBits<T>(highHalf), 4 * sizeof(T));
    Test<T>("CountBits(" + type + ", max)", Math::CountBits<T>(maximal), 8 * sizeof(T));
  }

  template<class T>
  constexpr void ConstexprTestCountBits()
  {
    ConstexprTest("CountBits(zero)", Math::CountBits<T>(0), 0);
    ConstexprTest("CountBits(one)", Math::CountBits<T>(1), 1);
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    ConstexprTest("CountBits(low half)", Math::CountBits<T>(lowHalf), 4 * sizeof(T));
    ConstexprTest("CountBits(high half)", Math::CountBits<T>(highHalf), 4 * sizeof(T));
    ConstexprTest("CountBits(max)", Math::CountBits<T>(maximal), 8 * sizeof(T));
  }

  template<class T>
  void TestScale(const std::string& test, T val, T inScale, T outScale, T result)
  {
    Test<T>("Scale " + test, Math::Scale(val, inScale, outScale), result);
    const Math::ScaleFunctor<T> scaler(inScale, outScale);
    Test<T>("ScaleFunctor " + test, scaler(val), result);
  }

  template<class T, T val, T inScale, T outScale, T result>
  constexpr void ConstexprTestScale()
  {
    constexpr const auto function = Math::Scale(val, inScale, outScale);
    ConstexprTest("Scale", function, result);
    constexpr const Math::ScaleFunctor<T> scaler(inScale, outScale);
    ConstexprTest("ScaleFunctor", scaler(val), result);
  }
}  // namespace

int main()
{
  try
  {
    std::cout << "---- Test for Log2 -----" << std::endl;
    {
      TestLog2<uint8_t>("uint8_t");
      TestLog2<uint16_t>("uint16_t");
      TestLog2<uint32_t>("uint32_t");
      TestLog2<uint64_t>("uint64_t");
      ConstexprTestLog2<uint8_t>();
      ConstexprTestLog2<uint16_t>();
      ConstexprTestLog2<uint32_t>();
      ConstexprTestLog2<uint64_t>();
    }
    std::cout << "---- Test for CountBits -----" << std::endl;
    {
      TestCountBits<uint8_t>("uint8_t");
      TestCountBits<uint16_t>("uint16_t");
      TestCountBits<uint32_t>("uint32_t");
      TestCountBits<uint64_t>("uint64_t");
      ConstexprTestCountBits<uint8_t>();
      ConstexprTestCountBits<uint16_t>();
      ConstexprTestCountBits<uint32_t>();
      ConstexprTestCountBits<uint64_t>();
    }
    std::cout << "---- Test for Scale -----" << std::endl;
    {
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

      ConstexprTestScale<uint32_t, 1000, 3000, 2000, 666>();
      ConstexprTestScale<uint32_t, 4000, 3000, 2000, 2666>();
      ConstexprTestScale<uint32_t, 1000, 2000, 3000, 1500>();
      ConstexprTestScale<uint32_t, 4000, 2000, 3000, 6000>();
      ConstexprTestScale<uint32_t, 10000, 30000, 20000, 6666>();
      ConstexprTestScale<uint32_t, 40000, 30000, 20000, 26666>();
      ConstexprTestScale<uint32_t, 10000, 20000, 30000, 15000>();
      ConstexprTestScale<uint32_t, 40000, 20000, 30000, 60000>();
      ConstexprTestScale<uint32_t, 1000000, 3000000, 2000000, 666666>();
      ConstexprTestScale<uint32_t, 4000000, 3000000, 2000000, 2666666>();

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
      TestScale<uint64_t>("uint64_t giant up", UINT64_C(10000000000), UINT64_C(30000000000), UINT64_C(20000000000),
                          UINT64_C(6666666666));
      TestScale<uint64_t>("uint64_t giant up overhead", UINT64_C(4000000000), UINT64_C(3000000000),
                          UINT64_C(20000000000), UINT64_C(26666666666));
      TestScale<uint64_t>("uint64_t giant down", UINT64_C(10000000000), UINT64_C(20000000000), UINT64_C(30000000000),
                          UINT64_C(15000000000));
      TestScale<uint64_t>("uint64_t giant down overhead", UINT64_C(4000000000), UINT64_C(2000000000),
                          UINT64_C(3000000000), UINT64_C(6000000000));

      ConstexprTestScale<uint64_t, 1000, 3000, 2000, 666>();
      ConstexprTestScale<uint64_t, 4000, 3000, 2000, 2666>();
      ConstexprTestScale<uint64_t, 1000, 2000, 3000, 1500>();
      ConstexprTestScale<uint64_t, 4000, 2000, 3000, 6000>();
      ConstexprTestScale<uint64_t, 10000, 30000, 20000, 6666>();
      ConstexprTestScale<uint64_t, 40000, 30000, 20000, 26666>();
      ConstexprTestScale<uint64_t, 10000, 20000, 30000, 15000>();
      ConstexprTestScale<uint64_t, 40000, 20000, 30000, 60000>();
      ConstexprTestScale<uint64_t, 1000000, 3000000, 2000000, 666666>();
      ConstexprTestScale<uint64_t, 4000000, 3000000, 2000000, 2666666>();
      ConstexprTestScale<uint64_t, 1000000, 2000000, 3000000, 1500000>();
      ConstexprTestScale<uint64_t, 4000000, 2000000, 3000000, 6000000>();
      ConstexprTestScale<uint64_t, UINT64_C(10000000000), UINT64_C(30000000000), UINT64_C(20000000000),
                         UINT64_C(6666666666)>();
      ConstexprTestScale<uint64_t, UINT64_C(4000000000), UINT64_C(3000000000), UINT64_C(20000000000),
                         UINT64_C(26666666666)>();
      ConstexprTestScale<uint64_t, UINT64_C(10000000000), UINT64_C(20000000000), UINT64_C(30000000000),
                         UINT64_C(15000000000)>();
      ConstexprTestScale<uint64_t, UINT64_C(4000000000), UINT64_C(2000000000), UINT64_C(3000000000),
                         UINT64_C(6000000000)>();
    }
  }
  catch (int code)
  {
    return code;
  }
}
