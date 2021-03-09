/**
 *
 * @file
 *
 * @brief  ZXZip test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.push_back("none.bin");    // 0
  tests.push_back("normal.bin");  // 3
  tests.push_back("fast.bin");    // 2
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
