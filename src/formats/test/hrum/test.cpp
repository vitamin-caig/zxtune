/**
 *
 * @file
 *
 * @brief  Hrum test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("packed1.bin");
  tests.emplace_back("packed2.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateHrumDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
