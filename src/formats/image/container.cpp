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
#include "container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>

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

      virtual const void* Start() const
      {
        return Delegate->Start();
      }

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      virtual std::size_t OriginalSize() const
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
        ? MakePtr<ImageContainer>(data, origSize)
        : Container::Ptr();
    }
    
    Container::Ptr CreateContainer(std::unique_ptr<Dump> data, std::size_t origSize)
    {
      const Binary::Container::Ptr container = Binary::CreateContainer(std::move(data));
      return CreateContainer(container, origSize);
    }
  }
}
