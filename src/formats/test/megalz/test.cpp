/**
 *
 * @file
 *
 * @brief  MegaLZ test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

int main()
{
  std::vector<std::string> tests;
  tests.emplace_back("greedy.bin");
  tests.emplace_back("optimal.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateMegaLZDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
