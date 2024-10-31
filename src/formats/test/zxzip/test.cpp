/**
 *
 * @file
 *
 * @brief  ZXZip test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/decoders.h"
#include "formats/test/utils.h"

#include "formats/packed.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("none.bin");    // 0
  tests.emplace_back("normal.bin");  // 3
  tests.emplace_back("fast.bin");    // 2
  // tests.push_back("slow.bin");   //1

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateZXZipDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
