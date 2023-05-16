/**
 *
 * @file
 *
 * @brief  Packed data container helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/container_factories.h>
// std includes
#include <cassert>

namespace Formats::Packed
{
  class PackedContainer : public Binary::BaseContainer<Container>
  {
  public:
    PackedContainer(Binary::Container::Ptr delegate, std::size_t origSize)
      : BaseContainer(std::move(delegate))
      , OriginalSize(origSize)
    {}

    std::size_t PackedSize() const override
    {
      return OriginalSize;
    }

  private:
    const std::size_t OriginalSize;
  };

  Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize)
  {
    return origSize && data && data->Size() ? MakePtr<PackedContainer>(std::move(data), origSize) : Container::Ptr();
  }
}  // namespace Formats::Packed
