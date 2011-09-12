#include "../utils.h"

int main()
{
  Dump etalon;
  Test::OpenFile("etalon.bin", etalon);
  Dump zip;
  //0ffsets:
  //0x14
  //0x403a
  //0x6095
  //0x80ef
  //0xa10d
  //0xc129
  //0xe148
  Test::OpenFile("test_v2.rar", zip);

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateRarDecoder();
    std::map<std::string, Dump> tests;
    const uint8_t* const data = &zip[0];
    tests["-m0"] = Dump(data + 0x14, data + 0x403a);
    tests["-m1"] = Dump(data + 0x403a, data + 0x6095);
    tests["-m2"] = Dump(data + 0x6095, data + 0x80ef);
    tests["-m3"] = Dump(data + 0x80ef, data + 0xa10d);
    tests["-m4"] = Dump(data + 0xa10d, data + 0xc129);
    tests["-m5"] = Dump(data + 0xc129, data + 0xe148);
    tests["-mm"] = Dump(data + 0xe148, data + 0x11578);
    Test::TestPacked(*decoder, etalon, tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
