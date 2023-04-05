/**
 *
 * @file
 *
 * @brief  Zip test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

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
    const auto packed = Formats::Packed::CreateZipDecoder();
    std::map<std::string, Binary::Container::Ptr> tests;
    tests["-p0"] = zip->GetSubcontainer(0, 0x4035);
    tests["-p1"] = zip->GetSubcontainer(0x4035, 0x6160 - 0x4035);
    tests["-p2"] = zip->GetSubcontainer(0x6160, 0x826e - 0x6160);
    tests["-p3"] = zip->GetSubcontainer(0x826e, 0xa36e - 0x826e);
    tests["-p4"] = zip->GetSubcontainer(0xa36e, 0xc433 - 0xa36e);
    tests["-p5"] = zip->GetSubcontainer(0xc433, 0xe4e9 - 0xc433);
    tests["-p6"] = zip->GetSubcontainer(0xe4e9, 0x105a2 - 0xe4e9);
    tests["-p7"] = zip->GetSubcontainer(0x105a2, 0x1265a - 0x105a2);
    tests["-p8"] = zip->GetSubcontainer(0x1265a, 0x1470b - 0x1265a);
    tests["-p9"] = zip->GetSubcontainer(0x1470b, 0x167bc - 0x1470b);
    Test::TestPacked(*packed, etalon, tests);

    const auto archived = Formats::Archived::CreateZipDecoder();
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
    Test::TestArchived(*archived, etalon, *zip, files);
  }

  void TestStreamed()
  {
    const auto archived = Formats::Archived::CreateZipDecoder();
    {
      std::vector<std::string> files;
      files.push_back("p0.bin");
      Test::TestArchived(*archived, "etalon.bin", "streamed_p0.zip", files);
    }
    {
      std::vector<std::string> files;
      files.push_back("p9.bin");
      Test::TestArchived(*archived, "etalon.bin", "streamed_p9.zip", files);
    }
    {
      std::vector<std::string> files;
      files.push_back("p9.zip");
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
