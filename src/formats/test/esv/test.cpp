/**
 *
 * @file
 *
 * @brief  ESVCruncher test
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
  tests.emplace_back("packed3.bin");
  tests.emplace_back("packed4.bin");
  tests.emplace_back("packed5.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateESVCruncherDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
