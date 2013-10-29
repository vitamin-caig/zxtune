/**
*
* @file
*
* @brief  Time test
*
* @author vitamin.caig@gmail.com
*
**/

#include <time/duration.h>
#include <time/oscillator.h>
#include <time/stamp.h>

#include <iostream>

namespace
{
  void Test(const std::string& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
      throw 1;
  }
  
  template<class T>
  void Test(const std::string& msg, T result, T reference)
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
}

int main()
{
  try
  {
    std::cout << "---- Test for time stamp ----" << std::endl;
    {
      Time::Nanoseconds ns(UINT64_C(10000000));
      {
        const Time::Nanoseconds ons(ns);
        Test<uint64_t>("Ns => Ns", ons.Get(), ns.Get());
        const Time::Microseconds ous(ns);
        Test<uint64_t>("Ns => Us", ous.Get(), ns.Get() / 1000);
        const Time::Milliseconds oms(ns);
        Test<uint64_t>("Ns => Ms", oms.Get(), ns.Get() / 1000000);
      }
      Time::Microseconds us(2000000);
      {
        const Time::Nanoseconds ons(us);
        Test<uint64_t>("Us => Ns", ons.Get(), uint64_t(us.Get()) * 1000);
        const Time::Microseconds ous(us);
        Test<uint64_t>("Us => Us", ous.Get(), us.Get());
        const Time::Milliseconds oms(us);
        Test<uint64_t>("Us => Ms", oms.Get(), us.Get() / 1000);
      }
      Time::Milliseconds ms(3000000);
      {
        const Time::Nanoseconds ons(ms);                         
        Test<uint64_t>("Ms => Ns", ons.Get(), uint64_t(ms.Get()) * 1000000);
        const Time::Microseconds ous(ms);
        Test<uint64_t>("Ms => Us", ous.Get(), ms.Get() * 1000);
        const Time::Milliseconds oms(ms);
        Test<uint64_t>("Ms => Ms", oms.Get(), ms.Get());
      }
      /*disabled due to float calculation threshold
      Time::Stamp<uint32_t, 1> s(10000);
      {
        const Time::Nanoseconds ons(s);                         
        Test<uint64_t>("S => Ns", ons.Get(), uint64_t(s.Get()) * UINT64_C(1000000000));
        const Time::Microseconds ous(s);
        Test<uint64_t>("S => Us", ous.Get(), uint64_t(s.Get()) * 1000000);
        const Time::Milliseconds oms(s);
        Test<uint64_t>("S => Ms", oms.Get(), s.Get() * 1000);
      }
      */
    }
    std::cout << "---- Test for time duration ----" << std::endl;
    {
      Time::Milliseconds period1(20), period2(30);
      Time::MillisecondsDuration msd1(12345, period1);
      Time::MillisecondsDuration msd2(5678, period2);
      //246900ms = 246.9s = 4m6s + 900ms/20
      Test<String>("MillisecondsDuration1 ToString()", msd1.ToString(), "4:06.45");
      //170340ms = 170.34s = 2m50s + 340ms/30
      Test<String>("MillisecondsDuration2 ToString()", msd2.ToString(), "2:50.11");
      Test("MillisecondsDuration compare", msd2 < msd1);
      msd1 += msd2;
      //period == 10
      //value == 12345 * 2 + 5678 * 3 = 41724
      //417240ms = 417.24s = 6m57s + 240/10
      Test<String>("MillisecondsDuration sum", msd1.ToString(), "6:57.24");
    }
  }
  catch (int code)
  {
    return code;
  }
}
