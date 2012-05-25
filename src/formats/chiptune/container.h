/*
Abstract:
  Chiptune container helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <crc.h>
#include <tools.h>
//library includes
#include <formats/chiptune.h>
//boost includes
#include <boost/make_shared.hpp>

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

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual const void* Data() const
      {
        return Delegate->Data();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
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

      virtual uint_t FixedChecksum() const
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

      virtual uint_t FixedChecksum() const
      {
        return Crc32(safe_ptr_cast<const uint8_t*>(Delegate->Data()) + FixedOffset, FixedSize);
      }
    private:
      const std::size_t FixedOffset;
      const std::size_t FixedSize;
    };

    inline Container::Ptr CreateKnownCrcContainer(Binary::Container::Ptr data, uint_t crc)
    {
      return data && data->Size()
        ? boost::make_shared<KnownCrcContainer>(data, crc)
        : Container::Ptr();
    }

    inline Container::Ptr CreateCalculatingCrcContainer(Binary::Container::Ptr data, std::size_t offset, std::size_t size)
    {
      return data && data->Size()
        ? boost::make_shared<CalculatingCrcContainer>(data, offset, size)
        : Container::Ptr();
    }
  }
}
