/**
 *
 * @file
 *
 * @brief  Metainfo operating implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/metainfo.h"

#include "binary/data_builder.h"
#include "binary/input_stream.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"

#include <cstring>
#include <map>
#include <utility>

namespace Formats::Chiptune
{
  class Patcher : public PatchedDataBuilder
  {
  public:
    explicit Patcher(Binary::View src)
      : Source(src)
    {}

    void InsertData(std::size_t offset, Binary::View data) override
    {
      Require(Insertions.insert(BlobsMap::value_type(offset, data)).second);
      SizeAddon += data.Size();
    }

    void OverwriteData(std::size_t offset, Binary::View data) override
    {
      Require(offset + data.Size() <= Source.Size());
      Require(Overwrites.insert(BlobsMap::value_type(offset, data)).second);
    }

    void FixLEWord(std::size_t offset, int_t delta) override
    {
      Require(offset + sizeof(uint16_t) <= Source.Size());
      Require(LEWordFixes.insert(FixesMap::value_type(offset, delta)).second);
    }

    Binary::Container::Ptr GetResult() const override
    {
      Binary::DataBuilder result;
      result.Add(Source);
      ApplyFixes(result);
      ApplyOverwrites(result);
      ApplyInsertions(result);
      return result.CaptureResult();
    }

  private:
    void ApplyFixes(Binary::DataBuilder& result) const
    {
      for (const auto& fix : LEWordFixes)
      {
        Fix<uint16_t>(result.Get(fix.first), fix.second);
      }
    }

    template<class T>
    static void Fix(void* data, int_t delta)
    {
      auto* ptr = safe_ptr_cast<LE<T>*>(data);
      *ptr = static_cast<T>(*ptr + delta);
    }

    void ApplyOverwrites(Binary::DataBuilder& result) const
    {
      for (const auto& over : Overwrites)
      {
        std::memcpy(result.Get(over.first), over.second.Start(), over.second.Size());
      }
    }

    void ApplyInsertions(Binary::DataBuilder& result) const
    {
      if (0 == SizeAddon)
      {
        return;
      }
      Binary::DataBuilder dst(result.Size() + SizeAddon);
      Binary::DataInputStream src(result.GetView());
      std::size_t oldOffset = 0;
      for (const auto& ins : Insertions)
      {
        if (const std::size_t toCopy = ins.first - oldOffset)
        {
          dst.Add(src.ReadData(toCopy));
          oldOffset += toCopy;
        }
        dst.Add(ins.second);
      }
      dst.Add(src.ReadRestData());
      result = std::move(dst);
    }

  private:
    const Binary::View Source;
    using BlobsMap = std::map<std::size_t, Binary::View>;
    using FixesMap = std::map<std::size_t, int_t>;
    BlobsMap Insertions;
    BlobsMap Overwrites;
    FixesMap LEWordFixes;
    std::size_t SizeAddon = 0;
  };
}  // namespace Formats::Chiptune

namespace Formats::Chiptune
{
  PatchedDataBuilder::Ptr PatchedDataBuilder::Create(Binary::View data)
  {
    return MakePtr<Patcher>(data);
  }
}  // namespace Formats::Chiptune
