#include "mkhebios.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdexcept>

void Check(bool condition, const char* msg)
{
  if (!condition)
  {
    throw std::runtime_error(msg);
  }
}

int main(int argc, const char* argv[])
{
  if (argc < 3)
  {
    std::cout << argv[0] << " <ps2bios> <txtdump>" << std::endl;
    return 1;
  }
  const std::string inputFilename(argv[1]);
  const std::string outputFilename(argv[2]);
  
  std::ifstream input(inputFilename.c_str(), std::ios::binary);
  input.seekg(0, std::ios::end);
  const int biosSize = input.tellg();
  input.seekg(0);
  std::vector<char> bios(biosSize);
  Check(input.read(bios.data(), biosSize), "Failed to read");
  
  int heBiosSize = biosSize;
  uint32_t* heBios = static_cast<uint32_t*>(::mkhebios_create(bios.data(), &heBiosSize));
  
  std::cout << "Size: " << biosSize << " -> " << heBiosSize << std::endl;
  
  Check(0 == heBiosSize % 4, "Invalid result size");
  
  std::ofstream output(outputFilename.c_str());
  
  output << std::hex << std::setfill('0');
  for (std::size_t idx = 0, lim = heBiosSize / 4; idx != lim; ++idx)
  {
    if (0 == idx % 8)
    {
      output << '\n';
    }
    output << "0x" << std::setw(8) << heBios[idx] << std::setw(-1) << ", ";
  }
  output << std::endl;
  
  ::mkhebios_delete(heBios);
  
  return 0;
}
