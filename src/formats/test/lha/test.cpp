#include "../utils.h"

namespace
{
  void TestArchive()
  {
    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("lh0");
    files.push_back("lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h0.lha", files);
  }

  void TestArchiveWithPath()
  {
    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("test/lh0");
    files.push_back("test/lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h1.lha", files);
  }

  void TestArchiveWithFolder()
  {
    const Formats::Archived::Decoder::Ptr archived = Formats::Archived::CreateLhaDecoder();
    std::vector<std::string> files;
    files.push_back("test/lh0");
    files.push_back("test/lh5");
    Test::TestArchived(*archived, "etalon.bin", "test_h2.lha", files);
  }
}

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
