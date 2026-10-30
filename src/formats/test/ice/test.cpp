/**
 *
 * @file
 *
 * @brief  ICE! test
 *
 **/

#include "formats/test/utils.h"

int main()
{
  try
  {
    const auto decoder = Formats::Packed::CreateICEDecoder();
    Test::TestPacked(*decoder, "etalon.bin", {"packed.bin"}, false);

    const auto packed = Test::OpenFile("packed.bin");
    Binary::DataBuilder input;
    input.Add(*packed);
    input.AddByte(0);
    const auto decoded = decoder->Decode(*input.CaptureResult());
    if (!decoded || decoded->PackedSize() != packed->Size())
    {
      throw std::runtime_error("Invalid used data size with trailing data");
    }
    std::cout << "  passed trailing data" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
