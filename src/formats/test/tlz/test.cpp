/**
 *
 * @file
 *
 * @brief  TLZ test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

namespace
{
  void TestTLZ1()
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateTurboLZDecoder();
    std::vector<std::string> tests;
    tests.emplace_back("packed1.bin");
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }

  void TestTLZ2()
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateTurboLZProtectedDecoder();
    std::vector<std::string> tests;
    tests.emplace_back("packed2.bin");
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
}  // namespace

int main()
{
  try
  {
    TestTLZ1();
    TestTLZ2();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
