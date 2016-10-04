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

//library includes
#include <binary/container.h>
//common includes
#include <contract.h>
#include <pointers.h>
#include <types.h>
//std includes
#include <algorithm>
#include <cstring>

namespace Binary
{
  //! @brief Sequental stream adapter
  class InputStream
  {
  public:
    explicit InputStream(const Container& rawData)
      : Data(rawData)
      , Start(static_cast<const uint8_t*>(rawData.Start()))
      , Finish(Start + Data.Size())
      , Cursor(Start)
    {
    }

    //! @brief Simple adapter to read specified type data
    template<class T>
    const T& ReadField()
    {
      return *safe_ptr_cast<const T*>(ReadData(sizeof(T)));
    }

    //! @brief Read ASCIIZ string with specified maximal size
    std::string ReadCString(std::size_t maxSize)
    {
      const uint8_t* const limit = std::min(Cursor + maxSize, Finish);
      const uint8_t* const strEnd = std::find(Cursor, limit, 0);
      Require(strEnd != limit);
      const std::string res(Cursor, strEnd);
      Cursor = strEnd + 1;
      return res;
    }

    //! @brief Read string till EOL
    std::string ReadString()
    {
      const uint8_t CR = 0x0d;
      const uint8_t LF = 0x0a;
      const uint8_t EOT = 0x00;
      static const uint8_t EOLCODES[3] = {CR, LF, EOT};

      Require(Cursor != Finish);
      const uint8_t* const eolPos = std::find_first_of(Cursor, Finish, EOLCODES, EOLCODES + 3);
      const uint8_t* nextLine = eolPos;
      if (nextLine != Finish && CR == *nextLine++)
      {
        if (nextLine != Finish && LF == *nextLine)
        {
          ++nextLine;
        }
      }
      const std::string result(Cursor, eolPos);
      Cursor = nextLine;
      return result;
    }

    //! @brief Read raw data
    const uint8_t* ReadData(std::size_t size)
    {
      Require(Cursor + size <= Finish);
      const uint8_t* const res = Cursor;
      Cursor += size;
      return res;
    }

    //! @brief Read rest data in source container
    Container::Ptr ReadRestData()
    {
      Require(Cursor < Finish);
      const std::size_t offset = GetPosition();
      const std::size_t size = GetRestSize();
      Cursor = Finish;
      return Data.GetSubcontainer(offset, size);
    }

    //! @brief Read as much data as possible
    std::size_t Read(void* buf, std::size_t len)
    {
      const std::size_t res = std::min(len, GetRestSize());
      std::memcpy(buf, ReadData(res), res);
      return res;
    }

    //! @brief Return data that is already read
    Container::Ptr GetReadData() const
    {
      return Data.GetSubcontainer(0, GetPosition());
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
    const Container& Data;
    const uint8_t* const Start;
    const uint8_t* const Finish;
    const uint8_t* Cursor;
  };
}
