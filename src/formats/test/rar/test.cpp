/**
 *
 * @file
 *
 * @brief  RAR test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

namespace
{
  void TestBase(const Binary::Dump& etalon)
  {
    std::cout << "Testing usual archive" << std::endl;
    // 0ffsets:
    // 0x14
    // 0x403a
    // 0x6095
    // 0x80ef
    // 0xa10d
    // 0xc129
    // 0xe148
    Binary::Dump rar;
    Test::OpenFile("test_v2.rar", rar);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateRarDecoder();
    std::map<std::string, Binary::Dump> tests;
    const uint8_t* const data = rar.data();
    tests["-m0"] = Binary::Dump(data + 0x14, data + 0x403a);
    tests["-m1"] = Binary::Dump(data + 0x403a, data + 0x6095);
    tests["-m2"] = Binary::Dump(data + 0x6095, data + 0x80ef);
    tests["-m3"] = Binary::Dump(data + 0x80ef, data + 0xa10d);
    tests["-m4"] = Binary::Dump(data + 0xa10d, data + 0xc129);
    tests["-m5"] = Binary::Dump(data + 0xc129, data + 0xe148);
    tests["-mm"] = Binary::Dump(data + 0xe148, data + 0x11578);
    Test::TestPacked(*packed, etalon, tests, false);

    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateRarDecoder();
    std::vector<std::string> files;
    files.push_back("p0.bin");
    files.push_back("p1.bin");
    files.push_back("p2.bin");
    files.push_back("p3.bin");
    files.push_back("p4.bin");
    files.push_back("p5.bin");
    files.push_back("pMM.bin");
    Test::TestArchived(*archived, "etalon.bin", "test_v2.rar", files);
  }

  void TestSolid(const Binary::Dump& etalon)
  {
    std::cout << "Testing solid archive" << std::endl;
    // Offsets:
    // 0x14
    // 0x2033
    Binary::Dump rar;
    Test::OpenFile("test_v2solid5.rar", rar);

    const Formats::Packed::Decoder::Ptr packed = Formats::Packed::CreateRarDecoder();
    std::map<std::string, Binary::Dump> tests;
    const uint8_t* const data = rar.data();
    tests["-p5"] = Binary::Dump(data + 0x14, data + 0x2033);
    tests["-p5solid"] = Binary::Dump(data + 0x2033, data + 0x2091);
    Test::TestPacked(*packed, etalon, tests, false);

    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateRarDecoder();
    std::vector<std::string> files;
    files.push_back("p5.bin");
    files.push_back("p5_solid.bin");
    Test::TestArchived(*archived, "etalon.bin", "test_v2solid5.rar", files);
  }
}  // namespace

int main()
{
  Binary::Dump etalon;
  Test::OpenFile("etalon.bin", etalon);

  try
  {
    TestBase(etalon);
    TestSolid(etalon);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
