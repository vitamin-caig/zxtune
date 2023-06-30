/**
 *
 * @file
 *
 * @brief  Common test code
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <pointers.h>
#include <types.h>
// library includes
#include <binary/data_builder.h>
#include <formats/archived/decoders.h>
#include <formats/packed/decoders.h>
#include <strings/split.h>
// std includes
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>

namespace Test
{
  // name is filename[:offset[:size]]
  inline Binary::Container::Ptr OpenFile(const std::string& name)
  {
    std::vector<std::string> elements;
    Strings::Split(name, ':', elements);
    elements.resize(3);
    const std::string& filename = elements.at(0);
    const std::string& offsetStr = elements.at(1);
    const std::string& sizeStr = elements.at(2);
    std::ifstream stream(filename.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    const std::size_t offset = offsetStr.empty() ? 0 : Strings::ConvertTo<std::size_t>(offsetStr);
    stream.seekg(0, std::ios_base::end);
    const std::size_t fileSize = stream.tellg();
    const std::size_t size = sizeStr.empty() ? fileSize - offset : Strings::ConvertTo<std::size_t>(sizeStr);
    stream.seekg(offset);
    Binary::DataBuilder tmp(size);
    stream.read(static_cast<char*>(tmp.Allocate(size)), size);
    if (!stream)
    {
      throw std::runtime_error("Failed to read from file");
    }
    return tmp.CaptureResult();
    // std::cout << "Read " << size << " bytes from " << name << std::endl;
  }

  inline void TestPacked(const Formats::Packed::Decoder& decoder, const Binary::Container& etalon,
                         const std::map<std::string, Binary::Container::Ptr>& tests, bool checkCorrupted = true)
  {
    std::cout << "Test for packed '" << decoder.GetDescription() << "'" << std::endl;
    for (const auto& test : tests)
    {
      const std::string& testname = test.first;
      const auto& testdata = test.second;
      std::cout << " testing " << testname << std::endl;
      const auto format = decoder.GetFormat();
      if (!format->Match(*testdata))
      {
        throw std::runtime_error("Failed to check for sanity.");
      }
      // positive test
      if (const auto unpacked = decoder.Decode(*testdata))
      {
        if (unpacked->Size() != etalon.Size() && 0 != std::memcmp(etalon.Start(), unpacked->Start(), etalon.Size()))
        {
          std::ofstream output((testname + "_decoded").c_str(), std::ios::binary);
          output.write(static_cast<const char*>(unpacked->Start()), unpacked->Size());
          std::ostringstream str;
          str << "Invalid decode:\n"
                 "ref size="
              << etalon.Size()
              << "\n"
                 "unpacked size="
              << unpacked->Size();
          throw std::runtime_error(str.str());
        }
        if (unpacked->PackedSize() != testdata->Size())
        {
          std::cout << "unpacked(" << unpacked->PackedSize() << ") != testdata(" << testdata->Size() << ")\n";
          throw std::runtime_error("Invalid used data size");
        }
        std::cout << "  passed positive" << std::endl;
      }
      else
      {
        throw std::runtime_error("Failed to decode");
      }
      if (const auto nonunpacked = decoder.Decode(etalon))
      {
        throw std::runtime_error("Unexpected success for invalid data");
      }
      else
      {
        std::cout << "  passed negative" << std::endl;
      }
      if (checkCorrupted)
      {
        Binary::DataBuilder corrupted;
        corrupted.Add(*testdata);
        for (std::size_t count = 0, size = corrupted.Size(); count != size * 7 / 100; ++count)
        {
          corrupted.Get<uint8_t>(rand() % size) ^= 0xff;
        }
        if (const auto nonunpacked = decoder.Decode(*corrupted.CaptureResult()))
        {
          throw std::runtime_error("Failed corrupted");
        }
        else
        {
          std::cout << "  passed corrupted" << std::endl;
        }
      }
    }
  }

  inline void TestPacked(const Formats::Packed::Decoder& decoder, const std::string& etalon,
                         const std::vector<std::string>& tests, bool checkCorrupted = true)
  {
    const auto reference = OpenFile(etalon);
    std::map<std::string, Binary::Container::Ptr> testData;
    for (const auto& test : tests)
    {
      testData[test] = OpenFile(test);
    }
    TestPacked(decoder, *reference, testData, checkCorrupted);
  }

  class ArchiveWalker : public Formats::Archived::Container::Walker
  {
  public:
    ArchiveWalker(const std::vector<std::string>& files, Binary::View etalon)
      : Files(files.begin(), files.end())
      , Etalon(etalon)
    {}

    void OnFile(const Formats::Archived::File& file) const override
    {
      std::cout << " checking " << file.GetName() << std::endl;
      if (Files.empty() || file.GetName() != Files.front())
      {
        throw std::runtime_error("Invalid files order");
      }
      if (file.GetSize() != Etalon.Size())
      {
        throw std::runtime_error("Invalid file size");
      }
      const auto unpacked = file.GetData();
      if (unpacked->Size() != Etalon.Size() || 0 != std::memcmp(Etalon.Start(), unpacked->Start(), unpacked->Size()))
      {
        std::ofstream output((Files.front() + "_decoded").c_str(), std::ios::binary);
        output.write(static_cast<const char*>(unpacked->Start()), unpacked->Size());
        std::ostringstream str;
        str << "Invalid decode:\n"
               "ref size="
            << Etalon.Size()
            << "\n"
               "unpacked size="
            << unpacked->Size();
        throw std::runtime_error(str.str());
      }
      Files.pop_front();
    }

  private:
    mutable std::list<std::string> Files;
    const Binary::View Etalon;
  };

  inline void TestArchived(const Formats::Archived::Decoder& decoder, const Binary::Container& reference,
                           const Binary::Container& archive, const std::vector<std::string>& testNames)
  {
    std::cout << "Test for container '" << decoder.GetDescription() << "'" << std::endl;

    const auto format = decoder.GetFormat();
    if (!format->Match(archive))
    {
      throw std::runtime_error("Failed to check for sanity.");
    }

    // positive test
    if (auto container = decoder.Decode(archive))
    {
      if (container->CountFiles() != testNames.size())
      {
        std::ostringstream res;
        res << "Files count mismatch (expected " << testNames.size() << " real " << container->CountFiles() << ")";
        throw std::runtime_error(res.str());
      }
      if (container->Size() != archive.Size())
      {
        std::ostringstream res;
        res << "Archive size mismatch (expected " << archive.Size() << " real " << container->Size() << ")";
        throw std::runtime_error(res.str());
      }
      const ArchiveWalker walker(testNames, reference);
      container->ExploreFiles(walker);
    }
    else
    {
      throw std::runtime_error("Failed to decode");
    }
  }

  inline void TestArchived(const Formats::Archived::Decoder& decoder, const std::string& etalon,
                           const std::string& archive, const std::vector<std::string>& testNames)
  {
    TestArchived(decoder, *OpenFile(etalon), *OpenFile(archive), testNames);
  }
}  // namespace Test
