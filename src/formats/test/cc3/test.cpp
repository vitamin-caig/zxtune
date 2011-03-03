#include <tools.h>
#include <formats/packed_decoders.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace
{
  void OpenFile(const std::string& name, Dump& result)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Dump tmp(size);
    stream.read(safe_ptr_cast<char*>(&tmp[0]), tmp.size());
    result.swap(tmp);
    //std::cout << "Read " << size << " bytes from " << name << std::endl;
  }

  void TestPacked(const Formats::Packed::Decoder& decoder, const std::string& etalon, const std::vector<std::string>& tests)
  {
    Dump reference;
    OpenFile(etalon, reference);
    for (std::vector<std::string>::const_iterator it = tests.begin(), lim = tests.end(); it != lim; ++it)
    {
      const std::string testname = *it;
      Dump test;
      OpenFile(testname, test);
      std::cout << " testing " << testname << std::endl;
      if (!decoder.Check(&test[0], test.size()))
      {
        throw std::runtime_error("Failed to check for sanity.");
      }
      std::size_t usedSize = 0;
      std::auto_ptr<Dump> unpacked = decoder.Decode(&test[0], test.size(), usedSize);
      if (!unpacked.get())
      {
        throw std::runtime_error("Failed to decode");
      }
      if (reference != *unpacked)
      {
        std::ofstream output((testname + "_decoded").c_str(), std::ios::binary);
        output.write(safe_ptr_cast<const char*>(&unpacked->front()), unpacked->size());
        std::ostringstream str;
        str << "Invalid decode:\n"
          "ref size=" << reference.size() << "\n"
          "unpacked size=" << unpacked->size();
        throw std::runtime_error(str.str());
      }
      if (usedSize != test.size())
      {
        throw std::runtime_error("Invalid used data size");
      }
    }
  }
}

int main()
{
  std::vector<std::string> tests;
  tests.push_back("packed.bin");

  try
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCodeCruncher3Decoder();
    TestPacked(*decoder, "etalon.bin", tests);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
