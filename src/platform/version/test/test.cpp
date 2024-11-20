/**
 *
 * @file
 *
 * @brief Test for version API
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "platform/version/api.h"

#include <iostream>

namespace Platform::Version
{
  const StringView PROGRAM_NAME = "Test";
}

int main()
{
  std::cout << Platform::Version::GetProgramVersionString() << std::endl;
}