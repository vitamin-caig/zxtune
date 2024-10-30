/**
 *
 * @file
 *
 * @brief Test for version API
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <iostream>
#include <platform/version/api.h>

namespace Platform::Version
{
  const StringView PROGRAM_NAME = "Test";
}

int main()
{
  std::cout << Platform::Version::GetProgramVersionString() << std::endl;
}