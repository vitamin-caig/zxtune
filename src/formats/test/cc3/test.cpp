/**
 *
 * @file
 *
 * @brief  CodeCruncher test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("packed.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCodeCruncher3Decoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
