#ifndef __VORTEX_IO_H_DEFINED__
#define __VORTEX_IO_H_DEFINED__

#include "vortex_base.h"

namespace ZXTune
{
  namespace Tracking
  {
    //Deserialization
    bool SampleLineFromString(const String& str, VortexPlayer::Sample::Line& line, bool& looped);

    template<class Iterator>
    Iterator SampleFromStrings(Iterator it, Iterator lim, VortexPlayer::Sample& sample)
    {
      VortexPlayer::Sample tmp;
      for (; it != lim; ++it)
      {
        if (it->empty())
        {
          continue;
        }
        tmp.Data.push_back(VortexPlayer::Sample::Line());
        bool loop(false);
        if (!SampleLineFromString(*it, tmp.Data.back(), loop))
        {
          return it;
        }
        if (loop)
        {
          tmp.Loop = tmp.Data.size() - 1;
        }
      }
      sample.Loop = tmp.Loop;
      sample.Data.swap(tmp.Data);
      return it;
    }

    bool OrnamentFromString(const String& str, VortexPlayer::Ornament& ornament);

    bool PatternLineFromString(const String& str, VortexPlayer::Line& line);

    template<class Iterator>
    Iterator PatternFromStrings(Iterator it, Iterator lim, VortexPlayer::Pattern& pat)
    {
      VortexPlayer::Pattern tmp;
      for (; it != lim; ++it)
      {
        if (it->empty())
        {
          continue;
        }
        tmp.push_back(VortexPlayer::Line());
        if (!PatternLineFromString(*it, tmp.back()))
        {
          return it;
        }
      }
      pat.swap(tmp);
      return it;
    }

    struct VortexDescr
    {
      VortexDescr() : Version(), Notetable(), Tempo(), Loop()
      {
      }
      String Title;
      String Author;
      std::size_t Version;
      std::size_t Notetable;
      std::size_t Tempo;
      std::size_t Loop;
      std::vector<std::size_t> Order;
    };

    bool PropertyFromString(const String& str, VortexDescr& descr);

    template<class Iterator>
    Iterator DescriptionFromStrings(Iterator it, Iterator lim, VortexDescr& descr)
    {
      VortexDescr tmp;
      for (; it != lim; ++it)
      {
        if (!PropertyFromString(*it, tmp))
        {
          return it;
        }
      }
      //TODO: perform checking
      descr.Title.swap(tmp.Title);
      descr.Author.swap(tmp.Author);
      descr.Version = tmp.Version;
      descr.Notetable = tmp.Notetable;
      descr.Tempo = tmp.Tempo;
      descr.Loop = tmp.Loop;
      descr.Order.swap(tmp.Order);
      return it;
    }

    //TODO serialization
  }
}

#endif //__VORTEX_IO_H_DEFINED__
