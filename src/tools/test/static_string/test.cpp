/**
 *
 * @file
 *
 * @brief  Compile-time string test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <static_string.h>
#include <string_view.h>
#include <types.h>
// std includes
#include <algorithm>
#include <cctype>
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

  char ToLower(char in)
  {
    return std::tolower(in);
  }

  String Lowercase(StringView s)
  {
    String res(s.size(), ' ');
    std::transform(s.begin(), s.end(), res.begin(), &ToLower);
    return res;
  }
}  // namespace

int main()
{
  try
  {
    constexpr auto str1 = "first"_ss;
    constexpr auto str2 = "second"_ss;
    constexpr auto str3 = "third"_ss;

    // Obfuscate reference in order to avoid compiler optimizations
    Test<StringView>("One string", str1, Lowercase("FIRST"));
    Test<StringView>("Two strings", str2 + str3, Lowercase("SECONDTHIRD"));
    Test<StringView>("Three strings", str1 + str3 + str2, Lowercase("FIRSTTHIRDSECOND"));
  }
  catch (...)
  {
    return 1;
  }
  return 0;
}
