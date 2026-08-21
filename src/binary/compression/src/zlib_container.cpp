/**
 *
 * @file
 *
 * @brief  On-demand uncompressing Binary::Container adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/compression/zlib_container.h"

#include "binary/compression/zlib.h"
#include "binary/container_factories.h"

#include "make_ptr.h"

namespace Binary::Compression::Zlib
{
  class DeferredDecompressContainer : public Container
  {
  public:
    DeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSize)
      : Packed(std::move(packed))
      , UnpackedSize(unpackedSize)
    {}

    const void* Start() const override
    {
      Unpack();
      return Unpacked->Start();
    }

    std::size_t Size() const override
    {
      if (UnpackedSize != 0)
      {
        return UnpackedSize;
      }
      else
      {
        Unpack();
        return Unpacked->Size();
      }
    }

    Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      Unpack();
      return Unpacked->GetSubcontainer(offset, size);
    }

  private:
    void Unpack() const
    {
      if (!Unpacked)
      {
        Unpacked = Decompress(*Packed, UnpackedSize);
        Packed.reset();
      }
    }

  private:
    mutable Data::Ptr Packed;
    mutable Ptr Unpacked;
    mutable std::size_t UnpackedSize;
  };

  Container::Ptr CreateDeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSize)
  {
    return MakePtr<DeferredDecompressContainer>(std::move(packed), unpackedSize);
  }
}  // namespace Binary::Compression::Zlib
