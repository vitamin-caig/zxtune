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
  extern const Char PROGRAM_NAME[] = {'T', 'e', 's', 't', 0};
}

int main()
{
  std::cout << Platform::Version::GetProgramVersionString() << std::endl;
}