#include "zxtune.h"
#include <contract.h>
#include <iostream>

int main(int argc, char* argv[])
{
  try
  {
    const char* const version = ZXTune_GetVersion();
    std::cout << "Testing for " << version << std::endl;
    const char* const path = argc > 1 ? argv[1] : "samples\\pt3\\Lat_mix2.pt3";
    std::cout << "Opening data" << std::endl;
    ZXTuneHandle data = ZXTune_OpenData(path);
    Require(data);
    std::cout << "Opening module" << std::endl;
    ZXTuneHandle module = ZXTune_OpenModule(data);
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
