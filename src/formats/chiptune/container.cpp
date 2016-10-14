/**
* 
* @file
*
* @brief  Chiptune container helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
//common includes
#include <crc.h>
#include <make_ptr.h>
//std includes
#include <cassert>

namespace Formats
{
  namespace Chiptune
  {
    class BaseDelegateContainer : public Container
    {
    public:
      explicit BaseDelegateContainer(Binary::Container::Ptr delegate)
        : Delegate(delegate)
      {
        assert(delegate && delegate->Size());
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

      uint_t Checksum() const override
      {
        return Crc32(static_cast<const uint8_t*>(Delegate->Start()), Delegate->Size());
      }
    protected:
      const Binary::Container::Ptr Delegate;
    };

    class KnownCrcContainer : public BaseDelegateContainer
    {
    public:
      KnownCrcContainer(Binary::Container::Ptr delegate, uint_t crc)
        : BaseDelegateContainer(delegate)
        , Crc(crc)
      {
      }

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
        : BaseDelegateContainer(delegate)
        , FixedOffset(offset)
        , FixedSize(size)
      {
      }

      uint_t FixedChecksum() const override
      {
        return Crc32(static_cast<const uint8_t*>(Delegate->Start()) + FixedOffset, FixedSize);
      }
    private:
      const std::size_t FixedOffset;
      const std::size_t FixedSize;
    };

    Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc)
    {
      return data && data->Size()
        ? MakePtr<KnownCrcContainer>(data, crc)
        : Container::Ptr();
    }

    Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size)
    {
      return data && data->Size()
        ? MakePtr<CalculatingCrcContainer>(data, offset, size)
        : Container::Ptr();
    }
  }
}
