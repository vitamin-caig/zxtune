/**
 *
 * @file
 *
 * @brief  DataSqueezer test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/decoders.h"
#include "formats/test/utils.h"

#include "formats/packed.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main()
{
  try
  {
    std::vector<std::string> tests;
    tests.emplace_back("4kfixed.bin");
    tests.emplace_back("win512.bin");
    tests.emplace_back("win1024.bin");
    tests.emplace_back("win2048.bin");
    // tests.push_back("win4096.bin");
    tests.emplace_back("win8192.bin");
    // tests.push_back("win16384.bin");
    tests.emplace_back("win32768.bin");
    tests.emplace_back("60005f00.bin");
    tests.emplace_back("60006100.bin");

    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateDataSquieezerDecoder();
    Test::TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
