/**
 *
 * @file
 *
 * @brief  LHA test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../utils.h"

namespace
{
  void TestArchive()
  {
    const auto archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("lh0");
    files.push_back("lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h0.lha", files);
  }

  void TestArchiveWithPath()
  {
    const auto archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("test/lh0");
    files.push_back("test/lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h1.lha", files);
  }

  void TestArchiveWithFolder()
  {
    const auto archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("test/lh0");
    files.push_back("test/lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h2.lha", files);
  }
}  // namespace

int main()
{
  try
  {
    TestArchive();
    TestArchiveWithPath();
    TestArchiveWithFolder();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
