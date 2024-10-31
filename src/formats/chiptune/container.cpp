/**
 *
 * @file
 *
 * @brief  Chiptune container helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/container.h"

#include <binary/container_base.h>
#include <binary/crc.h>

#include <make_ptr.h>

#include <cassert>
#include <utility>

namespace Formats::Chiptune
{
  class BaseDelegateContainer : public Binary::BaseContainer<Container>
  {
  public:
    explicit BaseDelegateContainer(Binary::Container::Ptr delegate)
      : BaseContainer(std::move(delegate))
    {}
  };

  class KnownCrcContainer : public BaseDelegateContainer
  {
  public:
    KnownCrcContainer(Binary::Container::Ptr delegate, uint_t crc)
      : BaseDelegateContainer(std::move(delegate))
      , Crc(crc)
    {}

    uint_t FixedChecksum() const override
    {
      return Crc;
    }

  private:
    const uint_t Crc;
  };

  class CalculatingCrcContainer : public BaseDelegateContainer
  {
  public:
    CalculatingCrcContainer(Binary::Container::Ptr delegate, std::size_t offset, std::size_t size)
      : BaseDelegateContainer(std::move(delegate))
      , FixedOffset(offset)
      , FixedSize(size)
    {}

    uint_t FixedChecksum() const override
    {
      return Binary::Crc32(Binary::View(*Delegate).SubView(FixedOffset, FixedSize));
    }

  private:
    const std::size_t FixedOffset;
    const std::size_t FixedSize;
  };

  Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc)
  {
    return data && data->Size() ? MakePtr<KnownCrcContainer>(std::move(data), crc) : Container::Ptr();
  }

  Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size)
  {
    return data && data->Size() ? MakePtr<CalculatingCrcContainer>(std::move(data), offset, size) : Container::Ptr();
  }
}  // namespace Formats::Chiptune
