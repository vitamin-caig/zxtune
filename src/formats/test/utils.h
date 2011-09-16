#ifndef FORMATS_TEST_UTILS_H
#define FORMATS_TEST_UTILS_H

#include <tools.h>
#include <types.h>
#include <formats/packed_decoders.h>
#include <map>
#include <cstring>
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
      const Binary::Format::Ptr format = decoder.GetFormat();
      if (!format->Match(&testdata[0], testdata.size()))
      {
        throw std::runtime_error("Failed to check for sanity.");
      }
      //positive test
      if (const Formats::Packed::Container::Ptr unpacked = decoder.Decode(&testdata[0], testdata.size()))
      {
        if (unpacked->Size() != etalon.size() &&
            0 != std::memcmp(&etalon[0], unpacked->Data(), etalon.size()))
        {
          std::ofstream output((testname + "_decoded").c_str(), std::ios::binary);
          output.write(static_cast<const char*>(unpacked->Data()), unpacked->Size());
          std::ostringstream str;
          str << "Invalid decode:\n"
            "ref size=" << etalon.size() << "\n"
            "unpacked size=" << unpacked->Size();
          throw std::runtime_error(str.str());
        }
        if (unpacked->PackedSize() != testdata.size())
        {
          throw std::runtime_error("Invalid used data size");
        }
        std::cout << "  passed positive" << std::endl;
      }
      else
      {
        throw std::runtime_error("Failed to decode");
      }
      if (const Formats::Packed::Container::Ptr nonunpacked = decoder.Decode(&etalon[0], etalon.size()))
      {
        throw std::runtime_error("Unexpected success for invalid data");
      }
      else
      {
        std::cout << "  passed negative" << std::endl;
      }
      /*
      Dump corrupted(testdata);
      corrupted[corrupted.size() / 4] ^= 0xff;
      corrupted[corrupted.size() / 2] ^= 0xff;
      corrupted[3 * corrupted.size() / 4] ^= 0xff;
      if (const Formats::Packed::Container::Ptr nonunpacked = decoder.Decode(&corrupted[0], corrupted.size()))
      {
        std::cout << "  failed corrupted" << std::endl;
      }
      */
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

