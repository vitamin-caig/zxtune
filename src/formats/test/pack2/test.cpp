/**
 *
 * @file
 *
 * @brief  Pack2 test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("packed.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreatePack2Decoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
