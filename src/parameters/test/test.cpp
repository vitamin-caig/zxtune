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
