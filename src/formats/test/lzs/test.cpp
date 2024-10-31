/**
 *
 * @file
 *
 * @brief  LZS test
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
  tests.emplace_back("packed.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateLZSDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
