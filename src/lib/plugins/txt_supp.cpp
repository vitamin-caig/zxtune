#include "plugin_enumerator.h"
#include "../io/container.h"
#include "vortex_io.h"

#include <error.h>
#include <tools.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include <boost/format.hpp>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>
#include <cassert>

#include <text/errors.h>
#include <text/plugins.h>

#define FILE_TAG 24577A20

namespace
{
  using namespace ZXTune;

  const String TEXT_TXT_VERSION(FromChar("Revision: $Rev$"));

  const std::size_t TEXT_MAX_SIZE = 1048576;//1M is more than enough

  inline bool FindSection(const String& str)
  {
    return str.size() >= 3 && str[0] == '[' && str[str.size() - 1] == ']';
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

  ////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class TXTPlayer : public Tracking::VortexPlayer
  {
    typedef Tracking::VortexPlayer Parent;
  public:
    TXTPlayer(const String& filename, const IO::DataContainer& data)
      : Parent(filename)
    {
      const String::value_type* const dataIt(static_cast<const String::value_type*>(data.Data()));
      const boost::iterator_range<const String::value_type*> range(dataIt, dataIt + data.Size() / sizeof(String::value_type));

      StringArray lines;
      boost::algorithm::split(lines, range, boost::algorithm::is_cntrl(),
        boost::algorithm::token_compress_on);

      Tracking::VortexDescr descr;

      Formatter fmt(TEXT_ERROR_INVALID_STRING);
      for (StringArray::const_iterator it = lines.begin(), lim = lines.end(); it != lim;)
      {
        const String& string(*it);
        const StringArray::const_iterator next(std::find_if(++it, lim, FindSection));
        std::size_t idx(0);
        if (Tracking::DescriptionHeaderFromString(string))
        {
          const StringArray::const_iterator stop(DescriptionFromStrings(it, next, descr));
          if (next != stop)
          {
            throw Error(ERROR_DETAIL, 1, (fmt % *stop).str());//TODO:code
          }
        }
        else if (Tracking::OrnamentHeaderFromString(string, idx))
        {
          Data.Ornaments.resize(idx + 1);
          if (!OrnamentFromString(*it, Data.Ornaments[idx]))
          {
            throw Error(ERROR_DETAIL, 1, (fmt % *it).str());//TODO:code
          }
        }
        else if (Tracking::SampleHeaderFromString(string, idx))
        {
          Data.Samples.resize(idx + 1);
          const StringArray::const_iterator stop(SampleFromStrings(it, next, Data.Samples[idx]));
          if (next != stop)
          {
            throw Error(ERROR_DETAIL, 1, (fmt % *stop).str());//TODO:code
          }
        }
        else if (Tracking::PatternHeaderFromString(string, idx))
        {
          Data.Patterns.resize(idx + 1);
          const StringArray::const_iterator stop(PatternFromStrings(it, next, Data.Patterns[idx]));
          if (next != stop)
          {
            throw Error(ERROR_DETAIL, 1, (fmt % *stop).str());//TODO:code
          }
        }
        else
        {
          throw Error(ERROR_DETAIL, 1, (fmt % string).str());//TODO
        }
        it = next;
      }

      Information.Statistic.Tempo = descr.Tempo;
      Information.Statistic.Position = descr.Order.size();
      Data.Positions.swap(descr.Order);
      Information.Loop = descr.Loop;
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), FixEntity<Parent::Ornament>);
      std::for_each(Data.Samples.begin(), Data.Samples.end(), FixEntity<Parent::Sample>);

      FillProperties((Formatter(TEXT_VORTEX_EDITOR) % int(descr.Version / 10) % int(descr.Version % 10)).str(),
        descr.Author, descr.Title, static_cast<const uint8_t*>(data.Data()), data.Size());
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
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW | CAP_CONV_VORTEX;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TXT_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TXT_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const String::value_type* const data(static_cast<const String::value_type*>(source.Data()));
    return Tracking::DescriptionHeaderFromString(String(data, std::find_if(data, data + source.Size(),
      static_cast<int(*)(int)>(&std::iscntrl))));
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create text player on invalid data");
    return ModulePlayer::Ptr(new TXTPlayer(filename, data));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
