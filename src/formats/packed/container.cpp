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
#include "container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>

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

      virtual std::size_t PackedSize() const
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
        ? MakePtr<PackedContainer>(data, origSize)
        : Container::Ptr();
    }

    Container::Ptr CreateContainer(std::auto_ptr<Dump> data, std::size_t origSize)
    {
      const Binary::Container::Ptr container = Binary::CreateContainer(data);
      return CreateContainer(container, origSize);
    }
  }
}
