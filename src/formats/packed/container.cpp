/**
* 
* @file
*
* @brief  Packed data container helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/packed/container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
//std includes
#include <cassert>

namespace Formats
{
  namespace Packed
  {
    class PackedContainer : public Container
    {
    public:
      PackedContainer(Binary::Container::Ptr delegate, std::size_t origSize)
        : Delegate(delegate)
        , OriginalSize(origSize)
      {
        assert(origSize && delegate && delegate->Size());
      }

      const void* Start() const override
      {
        return Delegate->Start();
      }

      std::size_t Size() const override
      {
        return Delegate->Size();
      }

      Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      std::size_t PackedSize() const override
      {
        return OriginalSize;
      }
    private:
      const Binary::Container::Ptr Delegate;
      const std::size_t OriginalSize;
    };
    
    Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize)
    {
      return origSize && data && data->Size()
        ? MakePtr<PackedContainer>(std::move(data), origSize)
        : Container::Ptr();
    }

    Container::Ptr CreateContainer(std::unique_ptr<Dump> data, std::size_t origSize)
    {
      auto container = Binary::CreateContainer(std::move(data));
      return CreateContainer(std::move(container), origSize);
    }
  }
}
