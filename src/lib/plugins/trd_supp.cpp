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

#define FILE_TAG A1239034

namespace
{
  using namespace ZXTune;

  const String TEXT_TRD_INFO("TRD modules support");
  const String TEXT_TRD_VERSION("0.1");

  const std::size_t TRD_MODULE_SIZE = 655360;
  const std::size_t BYTES_PER_SECTOR = 256;
  const std::size_t SECTORS_IN_TRACK = 16;
  const std::size_t MAX_FILES_COUNT = 128;
  const std::size_t SERVICE_SECTOR_NUM = 8;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif

  enum
  {
    NOENTRY = 0,
    DELETED = 1
  };
  PACK_PRE struct CatEntry
  {
    IO::TRDosName Name;
    uint16_t Length;
    uint8_t SizeInSectors;
    uint8_t Sector;
    uint8_t Track;

    std::size_t Offset() const
    {
      return BYTES_PER_SECTOR * (SECTORS_IN_TRACK * Track + Sector);
    }

    std::size_t Size() const
    {
      return SizeInSectors == ((fromLE(Length) - 1) / BYTES_PER_SECTOR) ?
        fromLE(Length) : BYTES_PER_SECTOR * SizeInSectors;
    }
  } PACK_POST;

  enum
  {
    TRD_ID = 0x10,

    DS_DD = 0x16,
    DS_SD = 0x17,
    SS_DD = 0x18,
    SS_SD = 0x19
  };
  PACK_PRE struct ServiceSector
  {
    uint8_t Zero;
    uint8_t Reserved1[224];
    uint8_t FreeSpaceSect;
    uint8_t FreeSpaceTrack;
    uint8_t Type;
    uint8_t Files;
    uint16_t FreeSectors;
    uint8_t ID;//0x10
    uint8_t Reserved2[12];
    uint8_t DeletedFiles;
    uint8_t Title[8];
    uint8_t Reserved3[3];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == 256);

  struct FileDescr
  {
    explicit FileDescr(const CatEntry& entry)
     : Name(GetFileName(entry.Name))
     , Offset(entry.Offset())
     , Size(entry.Size())
    {
    }

    bool IsMergeable(const CatEntry& rh)
    {
      return 0 == (Size % (BYTES_PER_SECTOR * 255)) && Offset + Size == rh.Offset();
    }

    void Merge(const CatEntry& rh)
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

  class TRDContainer : public MultitrackBase
  {
    class TRDIterator : public MultitrackBase::SubmodulesIterator
    {
    public:
      explicit TRDIterator(const IO::DataContainer& data) : Data(data)
      {
        const CatEntry* catEntry(static_cast<const CatEntry*>(data.Data()));
        for (std::size_t idx = 0; idx != MAX_FILES_COUNT && NOENTRY != *catEntry->Name.Filename; ++idx, ++catEntry)
        {
          if (DELETED != *catEntry->Name.Filename && catEntry->SizeInSectors)
          {
            if (Descrs.empty() || !Descrs.back().IsMergeable(*catEntry))
            {
              Descrs.push_back(FileDescr(*catEntry));
            }
            else
            {
              Descrs.back().Merge(*catEntry);
            }
          }
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
    TRDContainer(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
      : MultitrackBase(filename)
    {
      TRDIterator iterator(data);
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
    info.Capabilities = CAP_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TRD_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TRD_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(source.Size());
    if (limit < TRD_MODULE_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const ServiceSector* const sector(safe_ptr_cast<const ServiceSector*>(data + SERVICE_SECTOR_NUM * BYTES_PER_SECTOR));
    return sector->ID == TRD_ID && sector->Type == DS_DD;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create trd player on invalid data");
    return ModulePlayer::Ptr(new TRDContainer(filename, data, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
