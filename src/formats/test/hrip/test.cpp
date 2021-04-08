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
    Binary::Dump hrip;
    Test::OpenFile("test.hrp", hrip);

    Binary::Dump etalon;
    Test::OpenFile("etalon16.bin", etalon);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Binary::Dump> tests;
    const uint8_t* const data = hrip.data();
    tests["test16k"] = Binary::Dump(data + 8, data + 0x21c0);
    Test::TestPacked(*packed, etalon, tests);
  }

  void TestHripBig()
  {
    Binary::Dump hrip;
    Test::OpenFile("test.hrp", hrip);

    Binary::Dump etalon;
    Test::OpenFile("etalon48.bin", etalon);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Binary::Dump> tests;
    const uint8_t* const data = hrip.data();
    tests["test48k"] = Binary::Dump(data + 0x21c0, data + 0x4821);
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
