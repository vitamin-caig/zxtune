#include "plugin_enumerator.h"
#include "multitrack_supp.h"

#include "../io/container.h"
#include "../io/location.h"
#include "../../archives/formats/hrip.h"

#include <tools.h>
#include <error.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cctype>
#include <cassert>
#include <valarray>

#define FILE_TAG 942A64CC

namespace
{
  using namespace ZXTune;

  const String TEXT_HRP_INFO("HRiP modules support");
  const String TEXT_HRP_VERSION("0.1");

  const std::size_t HRP_MODULE_SIZE = 655360;

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class HripContainer : public MultitrackBase
  {
    class HripIterator : public SubmodulesIterator
    {
    public:
      explicit HripIterator(const IO::DataContainer& data)
        : Data(data)
      {
        Archive::CheckHrip(data.Data(), data.Size(), Files);
        Iterator = Files.end();
      }

      virtual void Reset()
      {
        Iterator = Files.begin();
      }
      virtual void Reset(const String& filename)
      {
        Iterator = std::find(Files.begin(), Files.end(), filename);
      }
      virtual bool Get(String& filename, IO::DataContainer::Ptr& container) const
      {
        while (Iterator != Files.end())
        {
          Dump dump;
          if (Archive::OK != Archive::DepackHrip(Data.Data(), Data.Size(), Iterator->Filename, dump))
          {
            break;
          }
          filename = Iterator->Filename;
          container = IO::DataContainer::Create(dump);
          return true;
        }
        return false;
      }
      virtual void Next()
      {
        if (Iterator != Files.end())
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
      Archive::HripFiles Files;
      Archive::HripFiles::const_iterator Iterator;
    };

  public:
    HripContainer(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
      : MultitrackBase(filename)
    {
      HripIterator iterator(data);
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
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_HRP_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_HRP_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    Archive::HripFiles files;
    return Archive::OK == Archive::CheckHrip(source.Data(), source.Size(), files) && !files.empty();
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create hrip player on invalid data");
    return ModulePlayer::Ptr(new HripContainer(filename, data, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
