#include "../utils.h"

namespace
{
  void TestLZH1()
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateLZH1Decoder();
    std::vector<std::string> tests;
    tests.push_back("packed1.bin");
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }

  void TestLZH2()
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateLZH2Decoder();
    std::vector<std::string> tests;
    tests.push_back("packed2.bin");
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
}


int main()
{
  try
  {
    TestLZH1();
    TestLZH2();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
