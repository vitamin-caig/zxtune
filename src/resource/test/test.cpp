#include <resource/api.h>
#include <error.h>
#include <tools.h>
#include <fstream>
#include <iostream>
#include <map>
#include <cstring>
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

  Dump OpenFile(const std::string& name)
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
    return tmp;
  }

  class LoadEachFileVisitor : public Resource::Visitor
  {
  public:
    LoadEachFileVisitor()
    {
      LoadFile("Makefile");
      LoadFile("nested/dir/file");
    }

    void OnResource(const String& name)
    {
      std::cout << "Found resource file " << name << std::endl;
      Test("Test file exists", 1 == Etalons.count(name));
      const Binary::Container::Ptr data = Resource::Load(name);
      const Dump& ref = Etalons[name];
      Test("Test file is expected", ref.size() == data->Size() && 0 == std::memcmp(&ref[0], data->Start(), ref.size()));
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
    std::map<String, Dump> Etalons;
  };
}

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
