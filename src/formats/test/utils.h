#ifndef FORMATS_TEST_UTILS_H
#define FORMATS_TEST_UTILS_H

#include <tools.h>
#include <types.h>
#include <formats/archived_decoders.h>
#include <formats/packed_decoders.h>
#include <list>
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

  void TestPacked(const Formats::Packed::Decoder& decoder, const Dump& etalonDump, const std::map<std::string, Dump>& tests, bool checkCorrupted = true)
  {
    std::cout << "Test for packed '" << decoder.GetDescription() << "'" << std::endl;
    const Binary::Container::Ptr etalon = Binary::CreateContainer(&etalonDump[0], etalonDump.size());
    for (std::map<std::string, Dump>::const_iterator it = tests.begin(), lim = tests.end(); it != lim; ++it)
    {
      const std::string& testname = it->first;
      const Dump& testdataDump = it->second;
      const Binary::Container::Ptr testdata = Binary::CreateContainer(&testdataDump[0], testdataDump.size());
      std::cout << " testing " << testname << std::endl;
      const Binary::Format::Ptr format = decoder.GetFormat();
      if (!format->Match(testdata->Data(), testdata->Size()))
      {
        throw std::runtime_error("Failed to check for sanity.");
      }
      //positive test
      if (const Formats::Packed::Container::Ptr unpacked = decoder.Decode(*testdata))
      {
        if (unpacked->Size() != etalon->Size() &&
            0 != std::memcmp(etalon->Data(), unpacked->Data(), etalon->Size()))
        {
          std::ofstream output((testname + "_decoded").c_str(), std::ios::binary);
          output.write(static_cast<const char*>(unpacked->Data()), unpacked->Size());
          std::ostringstream str;
          str << "Invalid decode:\n"
            "ref size=" << etalon->Size() << "\n"
            "unpacked size=" << unpacked->Size();
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
      if (const Formats::Packed::Container::Ptr nonunpacked = decoder.Decode(*etalon))
      {
        throw std::runtime_error("Unexpected success for invalid data");
      }
      else
      {
        std::cout << "  passed negative" << std::endl;
      }
      if (checkCorrupted)
      {
        std::auto_ptr<Dump> corruptedDump(new Dump(testdataDump));
        corruptedDump->at(corruptedDump->size() / 4) ^= 0xff;
        corruptedDump->at(corruptedDump->size() / 2) ^= 0xff;
        corruptedDump->at(3 * corruptedDump->size() / 4) ^= 0xff;
        const Binary::Container::Ptr corrupted = Binary::CreateContainer(corruptedDump);
        if (const Formats::Packed::Container::Ptr nonunpacked = decoder.Decode(*corrupted))
        {
          std::cout << "  failed corrupted" << std::endl;
        }
        else
        {
          std::cout << "  passed corrupted" << std::endl;
        }
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

  class ArchiveWalker : public Formats::Archived::Container::Walker
  {
  public:
    ArchiveWalker(const std::vector<std::string>& files, const Dump& etalon)
      : Files(files.begin(), files.end())
      , Etalon(etalon)
    {
    }

    virtual void OnFile(const Formats::Archived::File& file) const
    {
      std::cout << " checking " << file.GetName() << std::endl;
      if (Files.empty() || file.GetName() != Files.front())
      {
        throw std::runtime_error("Invalid files order");
      }
      if (file.GetSize() != Etalon.size())
      {
        throw std::runtime_error("Invalid file size");
      }
      const Binary::Container::Ptr unpacked = file.GetData();
      if (unpacked->Size() != Etalon.size() ||
          0 != std::memcmp(&Etalon[0], unpacked->Data(), unpacked->Size()))
      {
        std::ofstream output((Files.front() + "_decoded").c_str(), std::ios::binary);
        output.write(static_cast<const char*>(unpacked->Data()), unpacked->Size());
        std::ostringstream str;
        str << "Invalid decode:\n"
          "ref size=" << Etalon.size() << "\n"
          "unpacked size=" << unpacked->Size();
        throw std::runtime_error(str.str());
      }
      Files.pop_front();
    }
  private:
    mutable std::list<std::string> Files;
    const Dump& Etalon;
  };

  void TestArchived(const Formats::Archived::Decoder& decoder, const std::string& etalon, const std::string& test, const std::vector<std::string>& testNames)
  {
    Dump reference;
    OpenFile(etalon, reference);
    std::cout << "Test for container '" << decoder.GetDescription() << "'" << std::endl;
    Dump archive;
    OpenFile(test, archive);
    const Binary::Container::Ptr testData = Binary::CreateContainer(&archive[0], archive.size());

    const Binary::Format::Ptr format = decoder.GetFormat();
    if (!format->Match(testData->Data(), testData->Size()))
    {
      throw std::runtime_error("Failed to check for sanity.");
    }

    //positive test
    if (Formats::Archived::Container::Ptr container = decoder.Decode(*testData))
    {
      if (container->CountFiles() != testNames.size())
      {
        throw std::runtime_error("Files count mismatch");
      }
      if (container->Size() != archive.size())
      {
        throw std::runtime_error("Archive size mismatch");
      }
      ArchiveWalker walker(testNames, reference);
      container->ExploreFiles(walker);
    }
    else
    {
      throw std::runtime_error("Failed to decode");
    }
  }
}

#endif //FORMATS_TEST_UTILS_H

