#include "plugin_enumerator.h"
#include "../io/container.h"

#include <tools.h>
#include <error.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <numeric>

#include <text/common.h>
#include <text/errors.h>
#include <text/plugins.h>

#define FILE_TAG 42F35DB2

namespace
{
  using namespace ZXTune;

  const String TEXT_FDI_VERSION(FromChar("Revision: $Rev: $"));

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct FDIHeader
  {
    uint8_t ID[3];
    uint8_t ReadOnly;
    uint16_t Cylinders;
    uint16_t Sides;
    uint16_t TextOffset;
    uint16_t DataOffset;
    uint16_t InfoSize;
  } PACK_POST;

  PACK_PRE struct FDITrack
  {
    uint32_t Offset;
    uint16_t Reserved;
    uint8_t SectorsCount;
    PACK_PRE struct Sector
    {
      uint8_t Cylinder;
      uint8_t Head;
      uint8_t Number;
      uint8_t Size;
      uint8_t Flags;
      uint16_t Offset;
    } PACK_POST;
    Sector Sectors[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(FDIHeader) == 14);
  BOOST_STATIC_ASSERT(sizeof(FDITrack::Sector) == 7);
  BOOST_STATIC_ASSERT(sizeof(FDITrack) == 14);

  const std::size_t FDI_MAX_SIZE = 1048576;
  const uint8_t FDI_ID[] = {'F', 'D', 'I'};

  struct SectorDescr
  {
    SectorDescr() : Num(), Begin(), End()
    {
    }
    SectorDescr(std::size_t num, const uint8_t* beg, const uint8_t* end) : Num(num), Begin(beg), End(end)
    {
    }
    std::size_t Num;
    const uint8_t* Begin;
    const uint8_t* End;

    bool operator < (const SectorDescr& rh) const
    {
      return Num < rh.Num;
    }
  };

  //////////////////////////////////////////////////////////////////////////
  class FDIContainer : public ModulePlayer
  {
  public:
    FDIContainer(const String& filename, const IO::DataContainer& src, uint32_t capFilter)
    {
      const std::size_t limit(src.Size());
      const uint8_t* data(static_cast<const uint8_t*>(src.Data()));
      const FDIHeader* const header(safe_ptr_cast<const FDIHeader*>(data));

      const std::size_t dataOffset(fromLE(header->DataOffset));
      Dump buffer;
      buffer.reserve(src.Size() - dataOffset);
      const FDITrack* trackInfo(safe_ptr_cast<const FDITrack*>(data + sizeof(*header) + fromLE(header->InfoSize)));
      for (std::size_t cyl = 0; cyl != fromLE(header->Cylinders); ++cyl)
      {
        for (std::size_t sid = 0; sid != fromLE(header->Sides); ++sid)
        {
          typedef std::vector<SectorDescr> SectorDescrs;
          SectorDescrs sectors;
          //sectors.reserve(trackInfo->SectorsCount);
          for (std::size_t secNum = 0; secNum != trackInfo->SectorsCount; ++secNum)
          {
            const FDITrack::Sector* const sector(trackInfo->Sectors + secNum);
            const std::size_t secSize = 128 << sector->Size;
            //assert(sector->Head == sid);
            assert(sector->Cylinder == cyl);
            const std::size_t offset(dataOffset + fromLE(sector->Offset) + fromLE(trackInfo->Offset));
            if (offset + secSize > limit)
            {
              throw Error(ERROR_DETAIL, 1, TEXT_ERROR_CONTAINER_PLAYER);//TODO: code
            }
            sectors.push_back(SectorDescr(sector->Number, 
              data + offset, data + offset + secSize));
          }

          std::sort(sectors.begin(), sectors.end());
          for (SectorDescrs::const_iterator it = sectors.begin(), lim = sectors.end(); it != lim; ++it)
          {
            buffer.insert(buffer.end(), it->Begin, it->End);
          }
          trackInfo = safe_ptr_cast<const FDITrack*>(safe_ptr_cast<const uint8_t*>(trackInfo) + 
            sizeof(*trackInfo) + (trackInfo->SectorsCount - 1) * sizeof(trackInfo->Sectors));
        }
      }

      Delegate = ModulePlayer::Create(filename, 
        *IO::DataContainer::Create(buffer), capFilter);
      if (!Delegate.get())
      {
        throw Error(ERROR_DETAIL, 1, TEXT_ERROR_CONTAINER_PLAYER);//TODO: code
      }
    }
    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      assert(Delegate.get());
      return Delegate->GetInfo(info);
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      assert(Delegate.get());
      Delegate->GetModuleInfo(info);
      StringMap::iterator ctrIter(info.Properties.find(Module::ATTR_CONTAINER));
      if (ctrIter == info.Properties.end())
      {
        info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, TEXT_FDI_CONTAINER));
      }
      else
      {
        ctrIter->second = String(TEXT_FDI_CONTAINER) + TEXT_CONTAINER_DELIMITER + ctrIter->second;
      }
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      assert(Delegate.get());
      return Delegate->GetModuleState(timeState, trackState);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Delegate.get());
      return Delegate->GetSoundState(state);
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      assert(Delegate.get());
      return Delegate->RenderFrame(params, receiver);
    }

    /// Controlling
    virtual State Reset()
    {
      assert(Delegate.get());
      return Delegate->Reset();
    }

    virtual State SetPosition(std::size_t frame)
    {
      assert(Delegate.get());
      return Delegate->SetPosition(frame);
    }

    virtual void Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      assert(Delegate.get());
      return Delegate->Convert(param, dst);
    }

  private:
    ModulePlayer::Ptr Delegate;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_STOR_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_FDI_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_FDI_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source, uint32_t capFilter)
  {
    const std::size_t limit(source.Size());
    if (limit >= FDI_MAX_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const FDIHeader* const header(safe_ptr_cast<const FDIHeader*>(data));
    BOOST_STATIC_ASSERT(sizeof(header->ID) == sizeof(FDI_ID));
    return 0 == std::memcmp(header->ID, FDI_ID, sizeof(FDI_ID));
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create fdi player on invalid data");
    return ModulePlayer::Ptr(new FDIContainer(filename, data, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
