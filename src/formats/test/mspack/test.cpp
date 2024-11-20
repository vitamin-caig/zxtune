/**
 *
 * @file
 *
 * @brief  MsPack test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("slow.msp:229");
  tests.emplace_back("fast.msp:229");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateMSPackDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
