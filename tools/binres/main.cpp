#include <contract.h>
#include <pointers.h>
#include <binary/compress.h>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace
{
  void ReadFile(const std::string& name, Dump& result)
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
  }
  
  void WriteLine(const uint8_t* start, const uint8_t* end, std::ostream& stream)
  {
    for (; start != end; ++start)
    {
      stream << ' ' << uint_t(*start) << ',';
    }
    stream << std::endl;
  }
  
  void WriteFile(const Dump& content, const std::string& name)
  {
    std::ofstream stream(name.c_str());
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    const std::size_t BYTES_PER_LINE = 32;
    const uint8_t* data = &content.front();
    for (std::size_t idx = 0, lim = content.size(); idx < lim;)
    {
      const std::size_t portion = std::min(BYTES_PER_LINE,  lim - idx);
      WriteLine(data + idx, data + idx + portion, stream);
      idx += portion;
    }
  }
}

int main(int argc, char* argv[])
{
  static const std::string COMPRESS_FLAG("--compress");
  try
  {
    Require(argc > 2);
    const bool isCompressed = COMPRESS_FLAG == argv[1];
    Require(isCompressed == (argc == 4));
    const std::string input = argv[1 + isCompressed];
    const std::string output = argv[2 + isCompressed];
    Dump content;
    ReadFile(input, content);
    if (isCompressed)
    {
      Binary::Compression::Zlib::Compress(content, content);
    }
    WriteFile(content, output);
  }
  catch (const std::exception& e)
  {
    std::cout << *argv << " [" << COMPRESS_FLAG << "] " << "<input_file> <output_file>" << std::endl;
    return 1;
  }
}