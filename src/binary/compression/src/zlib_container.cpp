/**
 *
 * @file
 *
 * @brief  On-demand uncompressing Binary::Container adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib.h>
#include <binary/compression/zlib_container.h>
#include <binary/compression/zlib_stream.h>
#include <binary/container_factories.h>

namespace Binary::Compression::Zlib
{
  class DeferredDecompressContainer : public Container
  {
  public:
    DeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint)
      : Packed(std::move(packed))
      , UnpackedSize(unpackedSizeHint)
    {}

    const void* Start() const override
    {
      Unpack();
      return Unpacked->data();
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
        return Unpacked->size();
      }
    }

    Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      Unpack();
      return CreateContainer(Unpacked, offset, size);
    }

  private:
    void Unpack() const
    {
      if (!Unpacked)
      {
        Binary::DataInputStream in(*Packed);
        Binary::DataBuilder out(UnpackedSize);
        Binary::Compression::Zlib::Decompress(in, out, UnpackedSize);
        Require(!UnpackedSize || UnpackedSize == out.Size());
        Unpacked.reset(new Dump());
        out.CaptureResult(*Unpacked);  // TODO: Unpacked = out.CaptureResult();
        UnpackedSize = Unpacked->size();
        Packed.reset();
      }
    }

  private:
    mutable Data::Ptr Packed;
    mutable std::shared_ptr<Dump> Unpacked;
    mutable std::size_t UnpackedSize;
  };

  Container::Ptr CreateDeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint)
  {
    return MakePtr<DeferredDecompressContainer>(std::move(packed), unpackedSizeHint);
  }

  Container::Ptr Decompress(View packed, std::size_t unpackedSizeHint)
  {
    Binary::DataInputStream in(packed);
    Binary::DataBuilder out(unpackedSizeHint);
    Binary::Compression::Zlib::Decompress(in, out, unpackedSizeHint);
    Require(!unpackedSizeHint || unpackedSizeHint == out.Size());
    return out.CaptureResult();
  }
}  // namespace Binary::Compression::Zlib
