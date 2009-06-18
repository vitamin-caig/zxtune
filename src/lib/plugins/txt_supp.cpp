#include "plugin_enumerator.h"
#include "../io/container.h"
#include "vortex_io.h"

#include <tools.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include <boost/format.hpp>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>
#include <cassert>

namespace
{
  using namespace ZXTune;

  const String TEXT_TXT_INFO("Text (vortex) modules support");
  const String TEXT_TXT_VERSION("0.1");
  const String TEXT_VORTEX_TRACKER("Vortex Tracker (PT%1%.%2% compatible)");

  const std::size_t TEXT_MAX_SIZE = 1048576;//1M is more than enough

  const String::value_type SECTION_MODULE[] = {'[', 'M', 'o', 'd', 'u', 'l', 'e', ']', 0};

  void Describing(ModulePlayer::Info& info);

  inline bool FindSection(const String& str)
  {
    return str.size() >= 3 && str[0] == '[' && str[str.size() - 1] == ']';
  }

  inline bool FindSectionIt(StringArray::const_iterator it)
  {
    return it->size() >= 3 && (*it)[0] == '[' && (*it)[it->size() - 1] == ']';
  }

  inline bool IsSection(const String& templ, const String& str, std::size_t& idx)
  {
    return 0 == str.find(templ) && (idx = std::atoi(str.substr(templ.size()).c_str()), true);
  }

  inline bool IsOrnamentSection(const String& str, std::size_t& idx)
  {
    static const String templ("[Ornament");
    return IsSection(templ, str, idx);
  }

  inline bool IsSampleSection(const String& str, std::size_t& idx)
  {
    static const String templ("[Sample");
    return IsSection(templ, str, idx);
  }

  inline bool IsPatternSection(const String& str, std::size_t& idx)
  {
    static const String templ("[Pattern");
    return IsSection(templ, str, idx);
  }

  template<class T>
  void FixEntity(T& ent)
  {
    if (ent.Data.empty())
    {
      ent.Loop = 0;
      ent.Data.resize(1);
    }
  }

  class PlayerImpl : public Tracking::VortexPlayer
  {
    typedef Tracking::VortexPlayer Parent;
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data)
      : Parent()
    {
      //TODO: use iterators
      const String& asString(String(static_cast<const String::value_type*>(data.Data()), 
        data.Size() / sizeof(String::value_type)));

      Tracking::VortexDescr descr;

      StringArray lines;
      boost::algorithm::split(lines, asString, boost::algorithm::is_cntrl(), boost::algorithm::token_compress_on);

      for (StringArray::const_iterator it = lines.begin(), lim = lines.end(); it != lim;)
      {
        const String& string(*it);
        const StringArray::const_iterator next(std::find_if(++it, lim, FindSection));
        std::size_t idx(0);
        if (string == SECTION_MODULE)
        {
          DescriptionFromStrings(it, next, descr);
        }
        else if (IsOrnamentSection(string, idx))
        {
          Data.Ornaments.resize(idx + 1);
          OrnamentFromString(*it, Data.Ornaments[idx]);//TODO
        }
        else if (IsSampleSection(string, idx))
        {
          Data.Samples.resize(idx + 1);
          SampleFromStrings(it, next, Data.Samples[idx]);//TODO
        }
        else if (IsPatternSection(string, idx))
        {
          Data.Patterns.resize(idx + 1);
          PatternFromStrings(it, next, Data.Patterns[idx]);
        }
        it = next;
      }

      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, descr.Title));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_AUTHOR, descr.Author));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, 
        (boost::format(TEXT_VORTEX_TRACKER) % int(descr.Version / 10) % int(descr.Version % 10)).str()));
      Information.Statistic.Tempo = descr.Tempo;
      Information.Statistic.Position = descr.Order.size();
      Data.Positions.swap(descr.Order);
      Information.Loop = descr.Loop;
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), FixEntity<Parent::Ornament>);
      std::for_each(Data.Samples.begin(), Data.Samples.end(), FixEntity<Parent::Sample>);

      Parent::Initialize(descr.Version % 10, static_cast<NoteTable>(descr.Notetable));
    }

    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      return Describing(info);
    }
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TXT_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TXT_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit >= TEXT_MAX_SIZE)
    {
      return false;
    }
    const String::value_type* const data(static_cast<const String::value_type*>(source.Data()));
    return 0 == memcmp(data, SECTION_MODULE, sizeof(SECTION_MODULE) - 1);
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create text player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator txtReg(Checking, Creating, Describing);
}
