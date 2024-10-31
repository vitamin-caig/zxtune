/**
 *
 * @file
 *
 * @brief  Platform test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "platform/tools.h"

#include "contract.h"

#include <exception>
#include <iostream>
#include <string>

int main()
{
  try
  {
    const std::string currentPath = Platform::GetCurrentImageFilename();
    std::cout << "Current exe path: " << currentPath << std::endl;
    const std::string::size_type beginPos = currentPath.find("platform_test");
    Require(beginPos != currentPath.npos);
    Require(beginPos != 0);
    return 0;
  }
  catch (const std::exception&)
  {
    return -1;
  }
}