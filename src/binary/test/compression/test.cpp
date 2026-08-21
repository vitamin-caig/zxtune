/**
 *
 * @file
 *
 * @brief  Compression test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/compression/zlib.h"
#include "binary/data_builder.h"
#include "binary/input_stream.h"

#include "contract.h"
#include "error.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
#define ASSERT_EQ(ref, test, msg)                                                                                      \
  do                                                                                                                   \
  {                                                                                                                    \
    const auto refVal = (ref);                                                                                         \
    const auto testVal = (test);                                                                                       \
    if (refVal != testVal)                                                                                             \
    {                                                                                                                  \
      std::ostringstream str;                                                                                          \
      str << msg << ": " << #ref << "(=" << refVal << ") != " << #test << "(=" << testVal << ")" << std::endl;         \
      throw std::runtime_error(str.str());                                                                             \
    }                                                                                                                  \
  } while (false)

  void Check(const std::string& msg, Binary::View ref, Binary::View test)
  {
    std::cout << " test " << msg << std::endl;
    ASSERT_EQ(ref.Size(), test.Size(), msg);
    ASSERT_EQ(0, std::memcmp(ref.Start(), test.Start(), test.Size()), msg);
  }

  std::ostream& operator<<(std::ostream& s, const Error& err)
  {
    s << err.GetText();
    return s;
  }

  template<class E, class F>
  E CheckThrowing(const std::string& msg, F&& func)
  {
    std::cout << " test " << msg << ": ";
    try
    {
      func();
      throw std::runtime_error("not thrown expected exception\n");
    }
    catch (const E& err)
    {
      std::cout << "thrown > " << err << std::endl;
      return err;
    }
  }

  void TestZlib(std::size_t size = 100000)
  {
    using namespace Binary::Compression::Zlib;
    std::vector<uint32_t> data(size);
    for (std::size_t i = 0; i < size; ++i)
    {
      data[i] = static_cast<uint32_t>(i);
    }
    const Binary::View source(data);
    const auto packed = Compress(source);
    std::cout << "Compressed " << source.Size() << " -> " << packed->Size() << std::endl;
    const auto doTest = [&source, &packed](const std::string& msg, Binary::View test) {
      std::cout << msg << ": " << test.Size() << " bytes\n";
      if (const auto depacked = Decompress(test))
      {
        Check("decompress all", source, *depacked);
      }
      CheckThrowing<Error>("decompress more than expected", [&]() { Decompress(test, source.Size() - 1); });
      CheckThrowing<Error>("decompress less than expected", [&]() { Decompress(test, source.Size() + 1); });
      const auto raw = test.SubView(2);
      if (const auto depackedRaw = DecompressRaw(raw, source.Size()))
      {
        Check("decompress raw expected size", source, *depackedRaw);
      }
      CheckThrowing<Error>("decompress raw more than expected", [&]() { DecompressRaw(raw, source.Size() - 1); });
      CheckThrowing<Error>("decompress raw less than expected", [&]() { DecompressRaw(raw, source.Size() + 1); });
      {
        Binary::DataInputStream rawStream(raw);
        if (const auto depackedRaw = DecompressRaw(rawStream))
        {
          Check("decompress all raw from stream", source, *depackedRaw);
        }
        ASSERT_EQ(packed->Size() - 6, rawStream.GetPosition(), "header and footer ignored");
      }
    };

    const Binary::View tested(*packed);
    doTest("Exact input", tested);

    {
      Binary::DataBuilder calgary;
      calgary.Add(tested);
      calgary.Add(source);
      doTest("Input with tail", calgary.GetView());
    }
    std::cout << "Corrupted: " << tested.Size() << " bytes\n";
    {
      Binary::DataBuilder corrupted;
      corrupted.Add(tested);
      std::memset(corrupted.Get(1000), 0, 4);
      CheckThrowing<Error>("decompress all", [&]() { Decompress(corrupted.GetView()); });

      {
        Binary::DataBuilder output;
        CheckThrowing<Error>("decompress all to stream", [&]() { Decompress(corrupted.GetView(), output); });
        Check("partially decompressed", source.SubView(0, 2000), output.GetView().SubView(0, 2000));
      }
      {
        Binary::DataInputStream input(corrupted.GetView().SubView(2));
        CheckThrowing<Error>("decompress all raw from stream", [&]() { DecompressRaw(input); });
        ASSERT_EQ(0, input.GetPosition(), "not advanced");
      }
    }
  }

}  // namespace

int main()
{
  try
  {
    TestZlib();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what();
    return 1;
  }
  catch (const Error& e)
  {
    std::cout << e;
    return 1;
  }

  return 0;
}
