#ifndef FORMATS_TEST_UTILS_H
#define FORMATS_TEST_UTILS_H

#include <tools.h>
#include <types.h>
#include <formats/packed_decoders.h>
#include <map>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Test
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

  void TestPacked(const Formats::Packed::Decoder& decoder, const Dump& etalon, const std::map<std::string, Dump>& tests)
  {
    for (std::map<std::string, Dump>::const_iterator it = tests.begin(), lim = tests.end(); it != lim; ++it)
    {
      const std::string& testname = it->first;
      const Dump& testdata = it->second;
      std::cout << " testing " << testname << std::endl;
      const DataFormat::Ptr format = decoder.GetFormat();
      if (!format->Match(&testdata[0], testdata.size()))
      {
        throw std::runtime_error("Failed to check for sanity.");
      }
      std::size_t usedSize = 0;
      std::auto_ptr<Dump> unpacked = decoder.Decode(&testdata[0], testdata.size(), usedSize);
      if (!unpacked.get())
      {
        throw std::runtime_error("Failed to decode");
      }
      if (etalon != *unpacked)
      {
        std::ofstream output((testname + "_decoded").c_str(), std::ios::binary);
        output.write(safe_ptr_cast<const char*>(&unpacked->front()), unpacked->size());
        std::ostringstream str;
        str << "Invalid decode:\n"
          "ref size=" << etalon.size() << "\n"
          "unpacked size=" << unpacked->size();
        throw std::runtime_error(str.str());
      }
      if (usedSize != testdata.size())
      {
        throw std::runtime_error("Invalid used data size");
      }
    }
  }

  void TestPacked(const Formats::Packed::Decoder& decoder, const std::string& etalon, const std::vector<std::string>& tests)
  {
    Dump reference;
    OpenFile(etalon, reference);
    std::map<std::string, Dump> testData;
    for (std::vector<std::string>::const_iterator it = tests.begin(), lim = tests.end(); it != lim; ++it)
    {
      OpenFile(*it, testData[*it]);
    }
    TestPacked(decoder, reference, testData);  
  }

}

#endif //FORMATS_TEST_UTILS_H

