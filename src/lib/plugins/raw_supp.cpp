#include "plugin_enumerator.h"
#include "multitrack_supp.h"

#include "../io/container.h"
#include "../io/location.h"

#include <error.h>
#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>

#include <text/plugins.h>

#define FILE_TAG 4100118E

namespace
{
  using namespace ZXTune;

  const String TEXT_RAW_VERSION(FromChar("Revision: $Rev$"));

  const std::size_t MAX_MODULE_SIZE = 1048576;//1Mb
  const std::size_t SCAN_STEP = 256;
  const std::size_t MIN_SCAN_SIZE = 512;

  const std::size_t LIMITER = ~std::size_t(0);

  const String::value_type EXTENTION[] = {'.', 'r', 'a', 'w', '\0'};

  String MakeFilename(std::size_t offset)
  {
    OutStringStream str;
    str << offset << EXTENTION;
    return str.str();
  }

  bool ParseFilename(const String& filename, std::size_t& offset)
  {
    InStringStream str(filename);
    String ext;
    return (str >> offset) && (str >> ext) && ext == EXTENTION && 0 == (offset % SCAN_STEP);
  }

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class RawContainer : public MultitrackBase
  {
    class RawIterator : public MultitrackBase::SubmodulesIterator
    {
    public:
      explicit RawIterator(const IO::DataContainer& data) : Data(data), Offset(LIMITER)
      {
      }

      virtual void Reset()
      {
        Offset = 0;
      }

      virtual void Reset(const String& filename)
      {
        if (!ParseFilename(filename, Offset))
        {
          Offset = LIMITER;
        }
      }

      virtual bool Get(String& filename, IO::DataContainer::Ptr& container) const
      {
        if (LIMITER == Offset || Offset > Data.Size() - MIN_SCAN_SIZE)
        {
          return false;
        }
        filename = MakeFilename(Offset);
        container = Data.GetSubcontainer(Offset, Data.Size() - Offset);
        return true;
      }
      virtual void Next()
      {
        if (LIMITER != Offset)
        {
          Offset += SCAN_STEP;
        }
        else
        {
          assert(!"Invalid logic");
        }
      }

    private:
      const IO::DataContainer& Data;
      std::size_t Offset;
    };
  public:
    RawContainer(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
      : MultitrackBase(filename, TEXT_RAW_CONTAINER)
    {
      RawIterator iterator(data);
      Process(iterator, capFilter & ~CAP_STOR_SCANER);
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
    info.Capabilities = CAP_STOR_SCANER | CAP_STOR_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_RAW_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_RAW_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source, uint32_t capFilter)
  {
    if (!(capFilter & CAP_STOR_SCANER))
    {
      return false;//scaner is not allowed here
    }
    const std::size_t limit(std::min(source.Size(), MAX_MODULE_SIZE));
    StringArray pathes;
    IO::SplitPath(filename, pathes);
    std::size_t offset(0);
    if (ParseFilename(pathes.back(), offset))
    {
      return offset + MIN_SCAN_SIZE < limit;
    }
    if (limit >= MIN_SCAN_SIZE)
    {
      unsigned modules(0);
      for (std::size_t off = SCAN_STEP; off <= limit - MIN_SCAN_SIZE; off += SCAN_STEP)
      {
        const String& modPath(IO::CombinePath(filename, MakeFilename(off)));
        ModulePlayer::Info info;
        //disable selfmates while scanning
        if (ModulePlayer::Check(modPath, *source.GetSubcontainer(off, limit - off), info, 
          capFilter & ~CAP_STOR_SCANER))
        {
          ++modules;
        }
      }
      return modules > 0;
    }
    return false;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create raw player on invalid data");
    return ModulePlayer::Ptr(new RawContainer(filename, data, capFilter & ~CAP_STOR_SCANER));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
