/**
* 
* @file
*
* @brief Simple test for libzxtune
*
* @author vitamin.caig@gmail.com
*
**/


#include "zxtune.h"
#include <contract.h>
#include <pointers.h>
#include <types.h>
#include <iostream>
#include <fstream>

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
}

int main(int argc, char* argv[])
{
  try
  {
    const char* const version = ZXTune_GetVersion();
    std::cout << "Testing for " << version << std::endl;
    const char* const path = argc > 1 ? argv[1] : "../../../samples/chiptunes/AY-3-8910/pt3/Lat_mix2.pt3";
    std::cout << "Opening data" << std::endl;
    Dump dump;
    OpenFile(path, dump);
    Require(dump.size() != 0);
    std::cout << "Creating data" << std::endl;
    const ZXTuneHandle data = ZXTune_CreateData(&dump.front(), dump.size());
    Require(data);
    std::cout << "Opening module" << std::endl;
    const ZXTuneHandle module = ZXTune_OpenModule(data);
    Require(module);
    std::cout << "Creating player" << std::endl;
    ZXTuneHandle player = ZXTune_CreatePlayer(module);
    Require(player);
  }
  catch (const std::exception&)
  {
    std::cout << "Failed" << std::endl;
  }
  return 0;
}
