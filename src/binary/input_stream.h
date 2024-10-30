/**
 *
 * @file
 *
 * @brief  Input binary stream helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <binary/container.h>
#include <binary/view.h>
// common includes
#include <byteorder.h>
#include <contract.h>
#include <pointers.h>
#include <string_view.h>
#include <types.h>
// std includes
#include <algorithm>
#include <cstring>
#include <utility>

namespace Binary
{
  //! @brief Sequental stream adapter
  class DataInputStream
  {
  public:
    explicit DataInputStream(Binary::View data)
      : Start(static_cast<const uint8_t*>(data.Start()))
      , Finish(Start + data.Size())
      , Cursor(Start)
    {}

    //! @brief Simple adapter to read specified type data
    template<class T>
    const T& Read()
    {
      static_assert(sizeof(T) > 1, "Use ReadByte for bytes");
      return *safe_ptr_cast<const T*>(ReadRawData(sizeof(T)));
    }

    uint8_t ReadByte()
    {
      return *ReadRawData(1);
    }

    //! @brief Read ASCIIZ string with specified maximal size
    StringView ReadCString(std::size_t maxSize)
    {
      const auto* const limit = std::min(Cursor + maxSize, Finish);
      const auto* const strEnd = std::find(Cursor, limit, 0);
      Require(strEnd != limit);
      const auto* begin = std::exchange(Cursor, strEnd + 1);
      return MakeStringView(begin, strEnd);
    }

    //! @brief Read string till EOL
    StringView ReadString()
    {
      const uint8_t CR = 0x0d;
      const uint8_t LF = 0x0a;
      const uint8_t EOT = 0x00;
      static const uint8_t EOLCODES[3] = {CR, LF, EOT};

      Require(Cursor != Finish);
      const auto* const eolPos = std::find_first_of(Cursor, Finish, EOLCODES, EOLCODES + 3);
      const auto* nextLine = eolPos;
      if (nextLine != Finish && CR == *nextLine++)
      {
        if (nextLine != Finish && LF == *nextLine)
        {
          ++nextLine;
        }
      }
      const auto* start = std::exchange(Cursor, nextLine);
      return MakeStringView(start, eolPos);
    }

    template<class T>
    const T* PeekField() const
    {
      return safe_ptr_cast<const T*>(PeekRawData(sizeof(T)));
    }

    const uint8_t* PeekRawData(std::size_t size) const
    {
      if (size <= GetRestSize())
      {
        return Cursor;
      }
      else
      {
        return nullptr;
      }
    }

    //! @brief Read as much data as possible
    std::size_t Read(void* buf, std::size_t len)
    {
      const std::size_t res = std::min(len, GetRestSize());
      std::memcpy(buf, ReadRawData(res), res);
      return res;
    }

    View ReadData(std::size_t size)
    {
      return {ReadRawData(size), size};
    }

    View ReadRestData()
    {
      Require(Cursor < Finish);
      return ReadData(GetRestSize());
    }

    void Skip(std::size_t size)
    {
      Require(size <= GetRestSize());
      Cursor += size;
    }

    void Seek(std::size_t pos)
    {
      Require(pos <= std::size_t(Finish - Start));
      Cursor = Start + pos;
    }

    //! @brief Return absolute read position
    std::size_t GetPosition() const
    {
      return Cursor - Start;
    }

    //! @brief Return available data size
    std::size_t GetRestSize() const
    {
      return Finish - Cursor;
    }

  private:
    const uint8_t* ReadRawData(std::size_t size)
    {
      Require(size <= GetRestSize());
      const uint8_t* const res = Cursor;
      Cursor += size;
      return res;
    }

    static inline StringView MakeStringView(const uint8_t* start, const uint8_t* end)
    {
      using Char = StringView::value_type;
      static_assert(sizeof(Char) == sizeof(uint8_t), "Invalid char size");
      return ::MakeStringView(safe_ptr_cast<const Char*>(start), safe_ptr_cast<const Char*>(end));
    }

  protected:
    const uint8_t* const Start;
    const uint8_t* const Finish;
    const uint8_t* Cursor;
  };

  // TODO: rename
  class InputStream : public DataInputStream
  {
  public:
    explicit InputStream(const Container& rawData)
      : DataInputStream(rawData)
      , Data(rawData)
    {}

    Container::Ptr ReadContainer(std::size_t size)
    {
      Require(size <= GetRestSize());
      const std::size_t offset = GetPosition();
      Cursor += size;
      return Data.GetSubcontainer(offset, size);
    }

    //! @brief Read rest data in source container
    Container::Ptr ReadRestContainer()
    {
      Require(Cursor < Finish);
      const std::size_t offset = GetPosition();
      const std::size_t size = GetRestSize();
      Cursor = Finish;
      return Data.GetSubcontainer(offset, size);
    }

    //! @brief Return data that is already read
    Container::Ptr GetReadContainer() const
    {
      return Data.GetSubcontainer(0, GetPosition());
    }

  private:
    const Container& Data;
  };
}  // namespace Binary
