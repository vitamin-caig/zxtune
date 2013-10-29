/**
*
* @file
*
* @brief  Metainfo operating implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "metainfo.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/container_factories.h>
//std includes
#include <set>
#include <map>

namespace
{
  template<class T>
  void Fix(void* data, int_t delta)
  {
    T* const ptr = static_cast<T*>(data);
    const T val = fromLE(*ptr);
    const T fixedVal = static_cast<T>(val + delta);
    *ptr = fromLE(fixedVal);
  }

  class Patcher : public Formats::Chiptune::PatchedDataBuilder
  {
  public:
    explicit Patcher(const Binary::Container& src)
      : Source(src)
      , SizeAddon(0)
    {
    }

    virtual void InsertData(std::size_t offset, const Dump& data)
    {
      Require(Insertions.insert(BlobsMap::value_type(offset, data)).second);
      SizeAddon += data.size();
    }

    virtual void OverwriteData(std::size_t offset, const Dump& data)
    {
      Require(offset + data.size() <= Source.Size());
      Require(Overwrites.insert(BlobsMap::value_type(offset, data)).second);
    }

    virtual void FixLEWord(std::size_t offset, int_t delta)
    {
      Require(offset + sizeof(uint16_t) <= Source.Size());
      Require(LEWordFixes.insert(FixesMap::value_type(offset, delta)).second);
    }

    virtual Binary::Container::Ptr GetResult() const
    {
      const uint8_t* const srcData = static_cast<const uint8_t*>(Source.Start());
      std::auto_ptr<Dump> result(new Dump(srcData, srcData + Source.Size()));
      ApplyFixes(*result);
      ApplyOverwrites(*result);
      ApplyInsertions(*result);
      return Binary::CreateContainer(result);
    }
  private:
    void ApplyFixes(Dump& result) const
    {
      for (FixesMap::const_iterator it = LEWordFixes.begin(), lim = LEWordFixes.end(); it != lim; ++it)
      {
        Fix<uint16_t>(static_cast<void*>(&result[it->first]), it->second);
      }
    }

    void ApplyOverwrites(Dump& result) const
    {
      for (BlobsMap::const_iterator it = Overwrites.begin(), lim = Overwrites.end(); it != lim; ++it)
      {
        std::copy(it->second.begin(), it->second.end(), result.begin() + it->first);
      }
    }

    void ApplyInsertions(Dump& result) const
    {
      if (0 == SizeAddon)
      {
        return;
      }
      Dump tmp(result.size() + SizeAddon);
      Dump::const_iterator src = result.begin();
      const Dump::const_iterator srcEnd = result.end();
      Dump::iterator dst = tmp.begin();
      std::size_t oldOffset = 0;
      for (BlobsMap::const_iterator it = Insertions.begin(), lim = Insertions.end(); it != lim; ++it)
      {
        if (const std::size_t toCopy = it->first - oldOffset)
        {
          const Dump::const_iterator nextEnd = src + toCopy;
          dst = std::copy(src, nextEnd, dst);
          src = nextEnd;
          oldOffset += toCopy;
        }
        dst = std::copy(it->second.begin(), it->second.end(), dst);
      }
      std::copy(src, srcEnd, dst);
      result.swap(tmp);
    }
  private:
    const Binary::Container& Source;
    typedef std::map<std::size_t, Dump> BlobsMap;
    typedef std::map<std::size_t, int_t> FixesMap;
    BlobsMap Insertions;
    BlobsMap Overwrites;
    FixesMap LEWordFixes;
    std::size_t SizeAddon;
  };
}

namespace Formats
{
  namespace Chiptune
  {
    PatchedDataBuilder::Ptr PatchedDataBuilder::Create(const Binary::Container& data)
    {
      return PatchedDataBuilder::Ptr(new Patcher(data));
    }
  }
}
