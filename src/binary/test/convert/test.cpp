/**
 *
 * @file
 *
 * @brief  Base64 test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/base64.h>
#include <parameters/convert.h>
#include <strings/format.h>

#include <iostream>

std::ostream& operator<<(std::ostream& s, const Binary::Dump& d)
{
  s << Parameters::ConvertToString(d);
  return s;
}

namespace
{
  void Test(const std::string& msg, bool val)
  {
    std::cout << (val ? "Passed" : "Failed") << " test for " << msg << std::endl;
    if (!val)
    {
      throw 1;
    }
  }

  template<class T>
  void Test(const std::string& msg, const T& result, const T& reference)
  {
    if (result == reference)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << "\ngot: '" << result << "'\nexp: '" << reference << '\'' << std::endl;
      throw 1;
    }
  }

  const uint8_t REFERENCE[] = {0xa5, 0xaa, 0x5a, 0x5a, 0x55, 0xa5};

  void TestBase64(std::size_t refSize, const std::string& encodedRef)
  {
    const Binary::Dump ref(REFERENCE, REFERENCE + refSize);
    const std::string encoded = Binary::Base64::Encode(ref);
    Test(Strings::Format("encode for {} bytes", refSize), encoded, encodedRef);
    const auto decoded = Binary::Base64::Decode(encoded);
    Test(Strings::Format("decode for {} bytes", refSize), decoded, ref);
  }
}  // namespace

int main()
{
  try
  {
    // 10100101
    // 101001 010000
    // p      Q
    TestBase64(1, "pQ==");
    // 10100101 10101010
    // 101001 011010 101000
    // p      a      o
    TestBase64(2, "pao=");
    // 10100101 10101010 01011010
    // 101001 011010 101001 011010
    // p      a      p      a
    TestBase64(3, "papa");
    // ... 01011010
    // ... 010110 100000
    // ... W      g
    TestBase64(4, "papaWg==");
    // ... 01011010 01010101
    // ... 010110 100101 010100
    // ... W      l      U
    TestBase64(5, "papaWlU=");
    // ... 01011010 01010101 10100101
    // ... 010110 100101 010110 100101
    // ... W      l      W      l
    TestBase64(6, "papaWlWl");
  }
  catch (int code)
  {
    return code;
  }
}
