/**
 *
 * @file
 *
 * @brief  HRiP test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

namespace
{
  void TestHripSmall()
  {
    Dump rar;
    Test::OpenFile("test.hrp", rar);

    Dump etalon;
    Test::OpenFile("etalon16.bin", etalon);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Dump> tests;
    const uint8_t* const data = rar.data();
    tests["test16k"] = Dump(data + 8, data + 0x21c0);
    Test::TestPacked(*packed, etalon, tests);
  }

  void TestHripBig()
  {
    Dump rar;
    Test::OpenFile("test.hrp", rar);

    Dump etalon;
    Test::OpenFile("etalon48.bin", etalon);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Dump> tests;
    const uint8_t* const data = rar.data();
    tests["test48k"] = Dump(data + 0x21c0, data + 0x4821);
    Test::TestPacked(*packed, etalon, tests);
  }
}  // namespace

int main()
{
  try
  {
    TestHripSmall();
    TestHripBig();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
