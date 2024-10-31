/**
 *
 * @file
 *
 * @brief  Math library test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <math/scale.h>

#include <string_type.h>

#include <iostream>

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

  template<class T, T result, T reference>
  constexpr void Test()
  {
    static_assert(result == reference, "Failed");
  }

  // to disable constexpr context
  template<class T>
  auto NoConst(T val)
  {
    return val;
  }

  template<class T>
  void TestLog2(const std::string& type)
  {
    Test<T>("Log2(" + type + ", zero)", Math::Log2<T>(NoConst(0)), 1);
    Test<T, Math::Log2<T>(0), 1>();
    Test<T>("Log2(" + type + ", one)", Math::Log2<T>(NoConst(1)), 1);
    Test<T, Math::Log2<T>(1), 1>();
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    Test<T>("Log2(" + type + ", low half)", Math::Log2<T>(NoConst(lowHalf)), 4 * sizeof(T));
    Test<T, Math::Log2<T>(lowHalf), 4 * sizeof(T)>();
    Test<T>("Log2(" + type + ", high half)", Math::Log2<T>(NoConst(highHalf)), 8 * sizeof(T));
    Test<T, Math::Log2<T>(highHalf), 8 * sizeof(T)>();
    Test<T>("Log2(" + type + ", max)", Math::Log2<T>(NoConst(maximal)), 8 * sizeof(T));
    Test<T, Math::Log2<T>(maximal), 8 * sizeof(T)>();
  }

  template<class T>
  void TestCountBits(const std::string& type)
  {
    Test<T>("CountBits(" + type + ", zero)", Math::CountBits<T>(NoConst(0)), 0);
    Test<T, Math::CountBits<T>(0), 0>();
    Test<T>("CountBits(" + type + ", one)", Math::CountBits<T>(NoConst(1)), 1);
    Test<T, Math::CountBits<T>(1), 1>();
    const T maximal = ~T(0);
    const T lowHalf = maximal >> 4 * sizeof(T);
    const T highHalf = lowHalf ^ maximal;
    Test<T>("CountBits(" + type + ", low half)", Math::CountBits<T>(NoConst(lowHalf)), 4 * sizeof(T));
    Test<T, Math::CountBits<T>(lowHalf), 4 * sizeof(T)>();
    Test<T>("CountBits(" + type + ", high half)", Math::CountBits<T>(NoConst(highHalf)), 4 * sizeof(T));
    Test<T, Math::CountBits<T>(highHalf), 4 * sizeof(T)>();
    Test<T>("CountBits(" + type + ", max)", Math::CountBits<T>(NoConst(maximal)), 8 * sizeof(T));
    Test<T, Math::CountBits<T>(maximal), 8 * sizeof(T)>();
  }

  template<class T, T val, T inScale, T outScale, T result>
  void TestScale(const std::string& test)
  {
    Test<T>("Scale " + test, Math::Scale(NoConst(val), NoConst(inScale), NoConst(outScale)), result);
    Test<T, Math::Scale(val, inScale, outScale), result>();
    const Math::ScaleFunctor<T> runtimeScaler(NoConst(inScale), NoConst(outScale));
    Test<T>("ScaleFunctor " + test, runtimeScaler(NoConst(val)), result);
    constexpr const Math::ScaleFunctor<T> constexprScaler(inScale, outScale);
    Test<T, constexprScaler(val), result>();
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
    }
    std::cout << "---- Test for CountBits -----" << std::endl;
    {
      TestCountBits<uint8_t>("uint8_t");
      TestCountBits<uint16_t>("uint16_t");
      TestCountBits<uint32_t>("uint32_t");
      TestCountBits<uint64_t>("uint64_t");
    }
    std::cout << "---- Test for Scale -----" << std::endl;
    {
      TestScale<uint32_t, 1000, 3000, 2000, 666>("uint32_t small up");
      TestScale<uint32_t, 4000, 3000, 2000, 2666>("uint32_t small up overhead");
      TestScale<uint32_t, 1000, 2000, 3000, 1500>("uint32_t small down");
      TestScale<uint32_t, 4000, 2000, 3000, 6000>("uint32_t small down overhead");
      TestScale<uint32_t, 10000, 30000, 20000, 6666>("uint32_t medium up");
      TestScale<uint32_t, 40000, 30000, 20000, 26666>("uint32_t medium up overhead");
      TestScale<uint32_t, 10000, 20000, 30000, 15000>("uint32_t medium down");
      TestScale<uint32_t, 40000, 20000, 30000, 60000>("uint32_t medium down overhead");
      TestScale<uint32_t, 1000000, 3000000, 2000000, 666666>("uint32_t big up");
      TestScale<uint32_t, 4000000, 3000000, 2000000, 2666666>("uint32_t big up overhead");

      TestScale<uint64_t, 1000, 3000, 2000, 666>("uint64_t small up");
      TestScale<uint64_t, 4000, 3000, 2000, 2666>("uint64_t small up overhead");
      TestScale<uint64_t, 1000, 2000, 3000, 1500>("uint64_t small down");
      TestScale<uint64_t, 4000, 2000, 3000, 6000>("uint64_t small down overhead");
      TestScale<uint64_t, 10000, 30000, 20000, 6666>("uint64_t medium up");
      TestScale<uint64_t, 40000, 30000, 20000, 26666>("uint64_t medium up overhead");
      TestScale<uint64_t, 10000, 20000, 30000, 15000>("uint64_t medium down");
      TestScale<uint64_t, 40000, 20000, 30000, 60000>("uint64_t medium down overhead");
      TestScale<uint64_t, 1000000, 3000000, 2000000, 666666>("uint64_t big up");
      TestScale<uint64_t, 4000000, 3000000, 2000000, 2666666>("uint64_t big up overhead");
      TestScale<uint64_t, 1000000, 2000000, 3000000, 1500000>("uint64_t big down");
      TestScale<uint64_t, 4000000, 2000000, 3000000, 6000000>("uint64_t big down overhead");
      TestScale<uint64_t, 10000000000, 30000000000, 20000000000, 6666666666>("uint64_t giant up");
      TestScale<uint64_t, 4000000000, 3000000000, 20000000000, 26666666666>("uint64_t giant up overhead");
      TestScale<uint64_t, 10000000000, 20000000000, 30000000000, 15000000000>("uint64_t giant down");
      TestScale<uint64_t, 4000000000, 2000000000, 3000000000, 6000000000>("uint64_t giant down overhead");
    }
  }
  catch (int code)
  {
    return code;
  }
}
