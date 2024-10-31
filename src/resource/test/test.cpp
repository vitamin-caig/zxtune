/**
 *
 * @file
 *
 * @brief  Resource test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/container.h"
#include "binary/data_builder.h"
#include "resource/api.h"
#include "strings/map.h"

#include "error.h"

#include <cstring>
#include <fstream>  // IWYU pragma: keep
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
  void Test(const char* msg, bool cond)
  {
    std::cout << msg << (cond ? " passed" : " failed") << std::endl;
    if (!cond)
    {
      throw 1;
    }
  }

  template<class T>
  void TestEq(const char* msg, T ref, T res)
  {
    std::cout << msg;
    if (ref == res)
    {
      std::cout << " passed" << std::endl;
    }
    else
    {
      std::cout << " failed (ref=" << ref << " res=" << res << ")" << std::endl;
      throw 1;
    }
  }

  Binary::Data::Ptr OpenFile(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Binary::DataBuilder tmp(size);
    stream.read(static_cast<char*>(tmp.Allocate(size)), size);
    return tmp.CaptureResult();
  }

  class LoadEachFileVisitor : public Resource::Visitor
  {
  public:
    LoadEachFileVisitor()
    {
      LoadFile("file");
      LoadFile("nested/dir/file");
    }

    void OnResource(StringView name) override
    {
      std::cout << "Found resource file " << name << std::endl;
      Test("Test file exists", 1 == Etalons.count(name));
      const auto data = Resource::Load(name);
      const auto& ref = Etalons[name];
      TestEq("Test file is expected size", ref->Size(), data->Size());
      Test("Test file is expected content", 0 == std::memcmp(ref->Start(), data->Start(), ref->Size()));
      Etalons.erase(name);
    }

    void CheckEmpty()
    {
      Test("All files visited", Etalons.empty());
    }

  private:
    void LoadFile(const String& name)
    {
      Etalons[name] = OpenFile(name);
    }

  private:
    Strings::ValueMap<Binary::Data::Ptr> Etalons;
  };
}  // namespace

int main()
{
  try
  {
    LoadEachFileVisitor visitor;
    Resource::Enumerate(visitor);
    visitor.CheckEmpty();
  }
  catch (const Error& e)
  {
    std::cout << e.ToString();
    return 1;
  }
  catch (...)
  {
    return 1;
  }
}
