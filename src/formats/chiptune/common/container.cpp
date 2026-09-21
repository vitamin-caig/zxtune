/**
 *
 * @file
 *
 * @brief  Chiptune container helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/common/container.h"

#include "binary/container_base.h"
#include "binary/crc.h"

#include "make_ptr.h"

#include <cassert>
#include <optional>
#include <utility>

namespace Formats::Chiptune
{
  class BaseDelegateContainer : public Binary::BaseContainer<Container>
  {
  public:
    explicit BaseDelegateContainer(Binary::Container::Ptr delegate)
      : BaseContainer(std::move(delegate))
    {}

    uint_t Checksum() const override
    {
      return Binary::Crc32(*Delegate);
    }
  };

  class KnownCrcContainer : public BaseDelegateContainer
  {
  public:
    KnownCrcContainer(Binary::Container::Ptr delegate, uint_t crc)
      : BaseDelegateContainer(std::move(delegate))
      , FixedCrc(crc)
    {}

    uint_t FixedChecksum() const override
    {
      return FixedCrc;
    }

  private:
    const uint_t FixedCrc;
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

  class CachingCrcContainer : public BaseDelegateContainer
  {
  public:
    CachingCrcContainer(Binary::Container::Ptr delegate, std::size_t limit)
      : BaseDelegateContainer(std::move(delegate))
      , Limit(limit)
    {}

    uint_t Checksum() const override
    {
      if (!Cached)
      {
        Cached = Binary::Crc32(Binary::View(*Delegate).SubView(0, Limit));
      }
      return *Cached;
    }

    uint_t FixedChecksum() const override
    {
      return Checksum();
    }

  private:
    const std::size_t Limit;
    mutable std::optional<uint_t> Cached;
  };

  Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc)
  {
    return data && data->Size() ? MakePtr<KnownCrcContainer>(std::move(data), crc) : Container::Ptr();
  }

  Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size)
  {
    return data && data->Size() ? MakePtr<CalculatingCrcContainer>(std::move(data), offset, size) : Container::Ptr();
  }

  Container::Ptr CreateCalculatingCrcContainer(const Binary::Container& data, std::size_t crcCalculatingLimit)
  {
    if (const auto size = data.Size())
    {
      auto region = data.GetSubcontainer(0, size);
      const auto effectiveLimit = crcCalculatingLimit ? std::min(size, crcCalculatingLimit) : size;
      return MakePtr<CachingCrcContainer>(std::move(region), effectiveLimit);
    }
    return {};
  }
}  // namespace Formats::Chiptune
