/**
*
* @file
*
* @brief  On-demand uncompressing Binary::Container adapter implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/compress.h>
#include <binary/compressed_data.h>
#include <binary/container_factories.h>

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      class DeferredUncompressContainer : public Container
      {
      public:
        DeferredUncompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint)
          : Packed(std::move(packed))
          , UnpackedSize(unpackedSizeHint)
        {
        }

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
            Unpacked.reset(new Dump(Binary::Compression::Zlib::Uncompress(Packed->Start(), Packed->Size())));
            Packed.reset();
            Require(!UnpackedSize || UnpackedSize == Unpacked->size());
            UnpackedSize = Unpacked->size();
          }
        }
      private:
        mutable Data::Ptr Packed;
        mutable std::shared_ptr<Dump> Unpacked;
        mutable std::size_t UnpackedSize;
      };
      
      Container::Ptr CreateDeferredUncompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint)
      {
        return MakePtr<DeferredUncompressContainer>(std::move(packed), unpackedSizeHint);
      }
    }
  }
}
