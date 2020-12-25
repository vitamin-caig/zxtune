/**
*
* @file
*
* @brief  Parameters test
*
* @author vitamin.caig@gmail.com
*
**/

#include <parameters/types.h>

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
}

int main()
{
  try
  {
  std::cout << "---- Test for Parameters::NameType" << std::endl;
  {
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
  }
  catch (int code)
  {
    return code;
  }
}
