/**
 *
 * @file
 *
 * @brief  Zip test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/test/utils.h"

namespace
{
  void TestRegular(const Binary::Container& etalon)
  {
    const auto zip = Test::OpenFile("test.zip");
    // 0ffsets:
    // 0
    // 0x4035
    // 0x6160
    // 0x826e
    // 0xa36e
    // 0xc433
    // 0xe4e9
    // 0x105a2
    // 0x1265a
    // 0x1470b
    const auto archived = Formats::Archived::CreateZipDecoder();
    std::vector<std::string> files;
    files.emplace_back("p0.bin");
    files.emplace_back("p1.bin");
    files.emplace_back("p2.bin");
    files.emplace_back("p3.bin");
    files.emplace_back("p4.bin");
    files.emplace_back("p5.bin");
    files.emplace_back("p6.bin");
    files.emplace_back("p7.bin");
    files.emplace_back("p8.bin");
    files.emplace_back("p9.bin");
    Test::TestArchived(*archived, etalon, *zip, files);
  }

  void TestStreamed()
  {
    const auto archived = Formats::Archived::CreateZipDecoder();
    {
      std::vector<std::string> files;
      files.emplace_back("p0.bin");
      Test::TestArchived(*archived, "etalon.bin", "streamed_p0.zip", files);
    }
    {
      std::vector<std::string> files;
      files.emplace_back("p9.bin");
      Test::TestArchived(*archived, "etalon.bin", "streamed_p9.zip", files);
    }
    {
      std::vector<std::string> files;
      files.emplace_back("p9.zip");
      Test::TestArchived(*archived, "streamed_p9.zip", "streamed_p9_p0.zip", files);
    }
  }
}  // namespace

int main()
{
  try
  {
    const auto etalon = Test::OpenFile("etalon.bin");
    TestRegular(*etalon);
    TestStreamed();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
