/**
* 
* @file
*
* @brief  Implementation of ROM images required for MT32Emu library
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "roms.h"
//library includes
#include <binary/compress.h>

namespace
{
  const unsigned char CONTROL_ROM[] =
  {
#include "control.inc"  
  };
  
  const unsigned char PCM_ROM[] =
  {
#include "pcm.inc"
  };
  
  class CompressedFile : public MT32Emu::File
  {
    const uint8_t* PackedData;
    const std::size_t PackedSize;
    Dump Content;
  public:
    CompressedFile(const unsigned char* packedData, size_t packedSize, std::size_t unpackedSize)
      : PackedData(packedData)
      , PackedSize(packedSize)
    {
      fileSize = unpackedSize;
    }

    virtual size_t getSize()
    {
      return fileSize;
    }
    
    virtual const unsigned char* getData()
    {
      if (!data)
      {
        Unpack();
      }
      return data;
    }
    
    virtual void close()
    {
    }
    
  private:
    void Unpack()
    {
      Content.resize(fileSize);
      uint8_t* const start = &Content.front();
      Binary::Compression::Zlib::Decompress(PackedData, PackedData + PackedSize, start, start + fileSize);
      data = &Content.front();
    }
  };
  
  void ReleaseFileAndROMImage(const MT32Emu::ROMImage* image)
  {
    delete image->getFile();
    MT32Emu::ROMImage::freeROMImage(image);
  }
  
  boost::shared_ptr<const MT32Emu::ROMImage> CreateROM(const uint8_t* data, std::size_t size, std::size_t unpackedSize)
  {
    std::auto_ptr<MT32Emu::File> file(new CompressedFile(data, size, unpackedSize));
    const boost::shared_ptr<const MT32Emu::ROMImage> result = boost::shared_ptr<const MT32Emu::ROMImage>(MT32Emu::ROMImage::makeROMImage(file.get()), &ReleaseFileAndROMImage);
    file.release();
    return result;
  }
}

namespace Devices
{
namespace MIDI
{
  namespace Detail
  {
    boost::shared_ptr<const MT32Emu::ROMImage> GetControlROM()
    {
      return CreateROM(CONTROL_ROM, sizeof(CONTROL_ROM), 65536);
    }
    
    boost::shared_ptr<const MT32Emu::ROMImage> GetPCMROM()
    {
      return CreateROM(PCM_ROM, sizeof(PCM_ROM), 524288);
    }
  }
}
}
