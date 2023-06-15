/**
 *
 * @file
 *
 * @brief  Time test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <static_string.h>
#include <time/duration.h>
#include <time/instant.h>
#include <time/serialize.h>

#include <iostream>

namespace
{
  void Test(const std::string& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
    {
      throw 1;
    }
  }

  template<class T1, class T2>
  void TestOrder(const std::string& msg, T1 lh, T2 rh)
  {
    Test(msg, lh < rh);
    Test("not " + msg, !(rh < lh));
  }

  // to disable constexpr context
  template<class T>
  auto NoConst(T val)
  {
    return val;
  }

#define ConstexprTestOrder(msg, lh, rh)                                                                                \
  {                                                                                                                    \
    static_assert(lh < rh, msg);                                                                                       \
    static_assert(!(rh < lh), "not " msg);                                                                             \
  }

  /* non-type template parameters of class type only available with C++20
  template<decltype(auto) lh, decltype(auto) rh>
  constexpr void TestOrder()
  {
    static_assert(lh < rh, "Failed");
    static_assert(!(rh < lh), "Failed");
  }
  */

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

  template<class T, T result, T reference>
  constexpr void Test()
  {
    static_assert(result == reference, "Failed");
  }

// double/float are not valid type for a template non-type parameter
#define TestFloat(msg, result, reference) static_assert(result == reference, msg)
}  // namespace

int main()
{
  try
  {
    std::cout << "---- Test for time instant ----" << std::endl;
    {
      constexpr const Time::AtNanosecond ns(10000000uLL);
      {
        constexpr const Time::AtNanosecond ons(ns);
        Test<uint64_t>("Ns => Ns", ons.Get(), ns.Get());
        Test<uint64_t, ons.Get(), ns.Get()>();
        // check out type matching
        constexpr const auto ous = ns.CastTo<Time::Microsecond>();
        Test<uint64_t>("Ns => Us", ous.Get(), ns.Get() / 1000);
        Test<uint64_t, ous.Get(), ns.Get() / 1000>();
        constexpr const auto oms = ns.CastTo<Time::Millisecond>();
        Test<uint64_t>("Ns => Ms", oms.Get(), ns.Get() / 1000000);
        Test<uint64_t, oms.Get(), ns.Get() / 1000000>();
      }
      constexpr const Time::AtMicrosecond us(2000000);
      {
        constexpr const Time::AtNanosecond ons(us);
        Test<uint64_t>("Us => Ns", ons.Get(), uint64_t(us.Get()) * 1000);
        Test<uint64_t, ons.Get(), uint64_t(us.Get()) * 1000>();
        constexpr const Time::AtMicrosecond ous(us);
        Test<uint64_t>("Us => Us", ous.Get(), us.Get());
        Test<uint64_t, ous.Get(), us.Get()>();
        constexpr const auto oms = us.CastTo<Time::Millisecond>();
        Test<uint64_t>("Us => Ms", oms.Get(), us.Get() / 1000);
        Test<uint64_t, oms.Get(), us.Get() / 1000>();
      }
      constexpr const Time::AtMillisecond ms(3000000);
      {
        constexpr const Time::AtNanosecond ons(ms);
        Test<uint64_t>("Ms => Ns", ons.Get(), uint64_t(ms.Get()) * 1000000);
        Test<uint64_t, ons.Get(), uint64_t(ms.Get()) * 1000000>();
        constexpr const Time::AtMicrosecond ous(ms);
        Test<uint64_t>("Ms => Us", ous.Get(), ms.Get() * 1000);
        Test<uint64_t, ous.Get(), ms.Get() * 1000>();
        constexpr const Time::AtMillisecond oms(ms);
        Test<uint64_t>("Ms => Ms", oms.Get(), ms.Get());
        Test<uint64_t, oms.Get(), ms.Get()>();
      }
      {
        constexpr const Time::Nanoseconds dns(4000000);
        {
          constexpr const auto ns_ns = ns + dns;
          static_assert(std::is_same<decltype(ns_ns), decltype(ns)>::value, "Wrong common type");
          Test<uint64_t>("Ns + ns", ns_ns.Get(), ns.Get() + dns.Get());
          Test<uint64_t, ns_ns.Get(), ns.Get() + dns.Get()>();

          constexpr const auto us_ns = us + dns;
          static_assert(std::is_same<decltype(us_ns), decltype(ns)>::value, "Wrong common type");
          Test<uint64_t>("Us + ns", us_ns.Get(), us.Get() * 1000 + dns.Get());
          Test<uint64_t, us_ns.Get(), us.Get() * 1000 + dns.Get()>();

          constexpr const auto ms_ns = ms + dns;
          static_assert(std::is_same<decltype(ms_ns), decltype(ns)>::value, "Wrong common type");
          Test<uint64_t>("Ms + ns", ms_ns.Get(), ms.Get() * 1000000uLL + dns.Get());
          Test<uint64_t, ms_ns.Get(), ms.Get() * 1000000uLL + dns.Get()>();
        }
        constexpr const Time::Microseconds dus(5000000);
        {
          constexpr const auto ns_us = ns + dus;
          static_assert(std::is_same<decltype(ns_us), decltype(ns)>::value, "Wrong common type");
          Test<uint64_t>("Ns + us", ns_us.Get(), ns.Get() + dus.Get() * 1000);
          Test<uint64_t, ns_us.Get(), ns.Get() + dus.Get() * 1000>();

          constexpr const auto us_us = us + dus;
          static_assert(std::is_same<decltype(us_us), decltype(us)>::value, "Wrong common type");
          Test<uint64_t>("Us + us", us_us.Get(), us.Get() + dus.Get());
          Test<uint64_t, us_us.Get(), us.Get() + dus.Get()>();

          constexpr const auto ms_us = ms + dus;
          static_assert(std::is_same<decltype(ms_us), decltype(us)>::value, "Wrong common type");
          Test<uint64_t>("Ms + us", ms_us.Get(), ms.Get() * 1000 + dus.Get());
          Test<uint64_t, ms_us.Get(), ms.Get() * 1000 + dus.Get()>();
        }
      }
    }
    std::cout << "---- Test for time duration ----" << std::endl;
    {
      constexpr const Time::Seconds s(123);
      constexpr const Time::Milliseconds ms(234567);
      constexpr const Time::Microseconds us(345678900);
      // 123s = 2m03s
      Test<String>("Seconds ToString()", Time::ToString(s), "2:03.00");
      // 234567ms = 234.567s = 3m54.56s
      Test<String>("Milliseconds ToString()", Time::ToString(ms), "3:54.56");
      // 345678900us = 345678ms = 345.67s = 5m45.67s
      Test<String>("Microseconds ToString()", Time::ToString(us), "5:45.67");
      TestOrder("s < ms", s, ms);
      ConstexprTestOrder("s < ms", s, ms);
      TestOrder("s < us", s, us);
      ConstexprTestOrder("s < us", s, us);
      TestOrder("ms < us", ms, us);
      ConstexprTestOrder("ms < us", ms, us);
      // 234.567s + 123s = 5m57.56s
      {
        const auto ms_s = ms + s;
        static_assert(std::is_same<decltype(ms_s), decltype(ms)>::value, "Wrong common type");
        Test<String>("ms+s ToString()", Time::ToString(ms_s), "5:57.56");
      }
      // 345.6789s + 123s = 7m48.67
      {
        const auto us_s = us + s;
        static_assert(std::is_same<decltype(us_s), decltype(us)>::value, "Wrong common type");
        Test<String>("us+s ToString()", Time::ToString(us_s), "7:48.67");
      }
      // 345.6789s + 234.567s = 9:40.24
      {
        const auto us_ms = us + ms;
        static_assert(std::is_same<decltype(us_ms), decltype(us)>::value, "Wrong common type");
        Test<String>("us+ms ToString()", Time::ToString(us_ms), "9:40.24");
      }
      {
        constexpr const auto period = Time::Microseconds::FromFrequency(50);
        Test<uint64_t>("P(50Hz)", period.Get(), 20000);
        Test<uint64_t, period.Get(), 20000>();
        Test<uint32_t>("F(25ms)", Time::Milliseconds(NoConst(25)).ToFrequency(), 40);
        Test<uint32_t, Time::Milliseconds(25).ToFrequency(), 40>();
        // Test<double>("3s / 2000ms", Time::Seconds(3).Divide<float>(Time::Milliseconds(2000)), 1.5f);
        // TestFloat("3s / 2000ms", Time::Seconds(3).Divide<float>(Time::Milliseconds(2000)), 1.5f);
        constexpr const auto done = Time::Microseconds::FromRatio(4762800, 44100);
        constexpr const auto total = Time::Milliseconds(180000);
        Test<uint64_t>("D(~4.7M@44100)", done.Get(), 108000000);
        Test<uint64_t, done.Get(), 108000000>();
        Test<float>("3m / ~4.7M@44100", done.Divide<float>(total), 0.6f);
        TestFloat("3m / ~4.7M@44100", done.Divide<float>(total), 0.6f);
        constexpr const auto doneMult = done * 100;
        Test<uint64_t, doneMult.PER_SECOND, done.PER_SECOND>();
        Test<uint64_t>("~4.7M@44100 * 100", doneMult.Get(), 10800000000);
        Test<uint64_t, doneMult.Get(), 10800000000>();
        Test<uint32_t>("3m / ~4.7M@44100, %", doneMult.Divide<uint32_t>(NoConst(total)), 60);
        Test<uint32_t, doneMult.Divide<uint32_t>(total), 60>();
        Test<uint32_t>("D(4.6M@44.1kHz)", Time::Milliseconds::FromRatio(NoConst(4685324), NoConst(44100)).Get(),
                       106243);
        Test<uint32_t, Time::Milliseconds::FromRatio(4685324, 44100).Get(), 106243>();
      }
    }
  }
  catch (int code)
  {
    return code;
  }
}
