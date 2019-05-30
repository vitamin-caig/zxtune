/**
*
* @file
*
* @brief  Image container helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/image/container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
//std includes
#include <cassert>

namespace Formats
{
  namespace Image
  {
    class ImageContainer : public Container
    {
    public:
      ImageContainer(Binary::Container::Ptr delegate, std::size_t origSize)
        : Delegate(delegate)
        , OrigSize(origSize)
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

      std::size_t OriginalSize() const override
      {
        return OrigSize;
      }
    private:
      const Binary::Container::Ptr Delegate;
      const std::size_t OrigSize;
    };

    Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize)
    {
      return origSize && data && data->Size()
        ? MakePtr<ImageContainer>(std::move(data), origSize)
        : Container::Ptr();
    }
    
    Container::Ptr CreateContainer(std::unique_ptr<Dump> data, std::size_t origSize)
    {
      auto container = Binary::CreateContainer(std::move(data));
      return CreateContainer(std::move(container), origSize);
    }
  }
}
