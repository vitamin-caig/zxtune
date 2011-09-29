#include "../utils.h"

int main()
{
  Dump etalon;
  Test::OpenFile("etalon.bin", etalon);
  Dump zip;
  //0ffsets:
  //0
  //0x4035
  //0x6160
  //0x826e
  //0xa36e
  //0xc433
  //0xe4e9
  //0x105a2
  //0x1265a
  //0x1470b
  Test::OpenFile("test.zip", zip);

  try
  {
    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateZipDecoder();
    std::map<std::string, Dump> tests;
    const uint8_t* const data = &zip[0];
    tests["-p0"] = Dump(data, data + 0x4035);
    tests["-p1"] = Dump(data + 0x4035, data + 0x6160);
    tests["-p2"] = Dump(data + 0x6160, data + 0x826e);
    tests["-p3"] = Dump(data + 0x826e, data + 0xa36e);
    tests["-p4"] = Dump(data + 0xa36e, data + 0xc433);
    tests["-p5"] = Dump(data + 0xc433, data + 0xe4e9);
    tests["-p6"] = Dump(data + 0xe4e9, data + 0x105a2);
    tests["-p7"] = Dump(data + 0x105a2, data + 0x1265a);
    tests["-p8"] = Dump(data + 0x1265a, data + 0x1470b);
    tests["-p9"] = Dump(data + 0x1470b, data + 0x167bc);
    Test::TestPacked(*packed, etalon, tests);

    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateZipDecoder();
    std::vector<std::string> files;
    files.push_back("p0.bin");
    files.push_back("p1.bin");
    files.push_back("p2.bin");
    files.push_back("p3.bin");
    files.push_back("p4.bin");
    files.push_back("p5.bin");
    files.push_back("p6.bin");
    files.push_back("p7.bin");
    files.push_back("p8.bin");
    files.push_back("p9.bin");
    Test::TestArchived(*archived, "etalon.bin", "test.zip", files);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
