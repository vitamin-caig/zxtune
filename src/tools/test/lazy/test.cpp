/**
 *
 * @file
 *
 * @brief  Lazy test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <lazy.h>
#include <make_ptr.h>
#include <string_type.h>
// std includes
#include <iostream>
#include <memory>

namespace
{
  void Test(const String& msg, bool condition)
  {
    if (condition)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << std::endl;
      throw 1;
    }
  }

  std::unique_ptr<int> IntPtrFactory(int* counter)
  {
    return std::make_unique<int>((*counter)++);
  }
}  // namespace

int main()
{
  try
  {
    // primitive types
    /*{
      int counter = 123;
      const Lazy<int> underTest([&counter]() { return counter++; });
      Test("Lazy int no calls", counter == 123 && !underTest);
      Test("Lazy int value", *underTest == 123 && underTest && counter == 124);
      Test("Lazy int value next calls", *underTest == 123 && underTest && counter == 124);
    }*/
    // unique_ptr
    {
      int counter = 345;
      const Lazy<std::unique_ptr<int>> underTest(&IntPtrFactory, &counter);
      Test("Lazy unique_ptr no calls", counter == 345 && !underTest);
      Test("Lazy unique_ptr value", *underTest == 345 && underTest && counter == 346);
      Test("Lazy unique_ptr value next calls", *underTest == 345 && underTest && counter == 346);
    }
    {
      struct Str
      {
        using Ptr = std::shared_ptr<const Str>;

        Str(int t)
          : Field(t)
        {}

        const int Field;
      };
      int counter = 456;
      const Lazy<Str::Ptr> underTest([&counter]() { return MakePtr<Str>(counter++); });
      Test("Lazy const shared_ptr no calls", counter == 456 && !underTest);
      Test("Lazy const shared_ptr value", underTest->Field == 456 && underTest && counter == 457);
      Test("Lazy const shared_ptr value next calls", underTest->Field == 456 && underTest && counter == 457);
    }
  }
  catch (...)
  {
    return 1;
  }
  return 0;
}
