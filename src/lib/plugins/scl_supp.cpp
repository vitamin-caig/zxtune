#include "plugin_enumerator.h"
#include "multitrack_supp.h"

#include "../io/container.h"
#include "../io/location.h"
#include "../io/trdos.h"

#include <tools.h>
#include <error.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cctype>
#include <cassert>
#include <numeric>

#define FILE_TAG 10F03BAF

namespace
{
  using namespace ZXTune;

  const String TEXT_SCL_INFO("SCL modules support");
  const String TEXT_SCL_VERSION("0.1");

  const String::value_type SCL_ID[] = {'S', 'C', 'L', 0};

  const std::size_t BYTES_PER_SECTOR = 256;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct SCLEntry
  {
    IO::TRDosName Name;
    uint16_t Length;
    uint8_t SizeInSectors;

    std::size_t Size() const
    {
      return SizeInSectors == ((fromLE(Length) - 1) / BYTES_PER_SECTOR) ?
        fromLE(Length) : BYTES_PER_SECTOR * SizeInSectors;
    }
  } PACK_POST;

  PACK_PRE struct SCLHeader
  {
    uint8_t ID[8];//'SINCLAIR'
    uint8_t BlocksCount;
    SCLEntry Blocks[1];
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t SINCLAIR_ID[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};

  const std::size_t SCL_MIN_SIZE = sizeof(SCLHeader) + 255 + 4;
  const std::size_t SCL_MODULE_SIZE = sizeof(SCLHeader) - sizeof(SCLEntry) + 
    255 * (sizeof(SCLEntry) + 0xff00) + 4;

  BOOST_STATIC_ASSERT(sizeof(SCLEntry) == 14);
  BOOST_STATIC_ASSERT(sizeof(SCLHeader) == 23);

  struct FileDescr
  {
    explicit FileDescr(const SCLEntry& entry, std::size_t offset)
      : Name(GetFileName(entry.Name))
      , Offset(offset)
      , Size(entry.Size())
    {
    }

    bool IsMergeable(const SCLEntry& /*rh*/)
    {
      return 0 == (Size % (BYTES_PER_SECTOR * 255));
    }

    void Merge(const SCLEntry& rh)
    {
      assert(IsMergeable(rh));
      Size += rh.Size();
    }

    bool operator == (const String& name) const
    {
      return Name == name;
    }

    String Name;
    std::size_t Offset;
    std::size_t Size;
  };

  typedef std::vector<FileDescr> FileDescriptions;

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class SCLContainer : public MultitrackBase
  {
    class SCLIterator : public MultitrackBase::SubmodulesIterator
    {
    public:
      explicit SCLIterator(const IO::DataContainer& data) : Data(data)
      {
        const SCLHeader* const header(static_cast<const SCLHeader*>(data.Data()));
        std::size_t offset(safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
          safe_ptr_cast<const uint8_t*>(header));
        for (std::size_t idx = 0; idx != header->BlocksCount; ++idx)
        {
          const SCLEntry& entry(header->Blocks[idx]);
          if (Descrs.empty() || !Descrs.back().IsMergeable(entry))
          {
            Descrs.push_back(FileDescr(entry, offset));
          }
          else
          {
            Descrs.back().Merge(entry);
          }
          offset += entry.SizeInSectors * BYTES_PER_SECTOR;
        }
        Iterator = Descrs.end();
      }

      virtual void Reset()
      {
        Iterator = Descrs.begin();
      }

      virtual void Reset(const String& filename)
      {
        Iterator = std::find(Descrs.begin(), Descrs.end(), filename);
      }

      virtual bool Get(String& filename, IO::DataContainer::Ptr& container) const
      {
        if (Iterator == Descrs.end())
        {
          return false;
        }
        filename = Iterator->Name;
        container = Data.GetSubcontainer(Iterator->Offset, Iterator->Size);
        return true;
      }
      virtual void Next()
      {
        if (Iterator != Descrs.end())
        {
          ++Iterator;
        }
        else
        {
          assert(!"Invalid logic");
        }
      }

    private:
      const IO::DataContainer& Data;
      FileDescriptions Descrs;
      FileDescriptions::const_iterator Iterator;
    };
  public:
    SCLContainer(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
      : MultitrackBase(filename, SCL_ID)
    {
      SCLIterator iterator(data);
      Process(iterator, capFilter);
    }

    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      if (!GetPlayerInfo(info))
      {
        Describing(info);
      }
    }
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_STOR_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_SCL_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_SCL_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(source.Size());
    if (limit < SCL_MIN_SIZE)
    {
      return false;
    }
    const SCLHeader* const header(safe_ptr_cast<const SCLHeader*>(source.Data()));
    //TODO: checksum???
    if (0 != std::memcmp(header->ID, SINCLAIR_ID, sizeof(SINCLAIR_ID)) ||
        limit < sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1))
    {
      return false;
    }
    const std::size_t allSectors = std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 
      std::size_t(0), 
      boost::bind(std::plus<std::size_t>(), _1, boost::bind<std::size_t>(&SCLEntry::SizeInSectors, _2)));
    return limit >= sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1) + 
      allSectors * BYTES_PER_SECTOR;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create scl player on invalid data");
    return ModulePlayer::Ptr(new SCLContainer(filename, data, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
