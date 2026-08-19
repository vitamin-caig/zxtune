/**
 *
 * @file
 *
 * @brief  RAR test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

namespace
{
  void TestBase(const Binary::Container& etalon)
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
    const auto rar = Test::OpenFile("test_v2.rar");
    const auto archived = Formats::Archived::CreateRarDecoder();
    std::vector<std::string> files;
    files.emplace_back("p0.bin");
    files.emplace_back("p1.bin");
    files.emplace_back("p2.bin");
    files.emplace_back("p3.bin");
    files.emplace_back("p4.bin");
    files.emplace_back("p5.bin");
    files.emplace_back("pMM.bin");
    Test::TestArchived(*archived, etalon, *rar, files);
  }

  void TestSolid(const Binary::Container& etalon)
  {
    std::cout << "Testing solid archive" << std::endl;
    // Offsets:
    // 0x14
    // 0x2033
    const auto rar = Test::OpenFile("test_v2solid5.rar");
    const auto archived = Formats::Archived::CreateRarDecoder();
    std::vector<std::string> files;
    files.emplace_back("p5.bin");
    files.emplace_back("p5_solid.bin");
    Test::TestArchived(*archived, etalon, *rar, files);
  }
}  // namespace

int main()
{
  const auto etalon = Test::OpenFile("etalon.bin");

  try
  {
    TestBase(*etalon);
    TestSolid(*etalon);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
