/**
 *
 * @file
 *
 * @brief  HRiP test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

namespace
{
  void TestHripSmall(const Binary::Container& hrip)
  {
    const auto etalon = Test::OpenFile("etalon16.bin");

    const auto packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Binary::Container::Ptr> tests;
    tests["test16k"] = hrip.GetSubcontainer(8, 0x21c0 - 8);
    Test::TestPacked(*packed, *etalon, tests);
  }

  void TestHripBig(const Binary::Container& hrip)
  {
    const auto etalon = Test::OpenFile("etalon48.bin");

    const auto packed = Formats::Packed::CreateHrust23Decoder();
    std::map<std::string, Binary::Container::Ptr> tests;
    tests["test48k"] = hrip.GetSubcontainer(0x21c0, 0x4821 - 0x21c0);
    Test::TestPacked(*packed, *etalon, tests);
  }
}  // namespace

int main()
{
  try
  {
    const auto hrip = Test::OpenFile("test.hrp");
    TestHripSmall(*hrip);
    TestHripBig(*hrip);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
