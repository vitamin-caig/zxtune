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

      bool PropertyFromString(const String& str);
      String PropertyToString(std::size_t idx) const;
    };


    template<class Iterator>
    Iterator DescriptionFromStrings(Iterator it, Iterator lim, VortexDescr& descr)
    {
      VortexDescr tmp;
      for (; it != lim; ++it)
      {
        if (!tmp.PropertyFromString(*it))
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

    //Serialization
    String SampleLineToString(const VortexPlayer::Sample::Line& line, bool looped);

    template<class Iterator>
    Iterator SampleToStrings(const VortexPlayer::Sample& sample, Iterator it)
    {
      std::size_t lpos(sample.Loop);
      for (std::vector<VortexPlayer::Sample::Line>::const_iterator sit = sample.Data.begin(), 
        slim = sample.Data.end(); sit != slim; ++sit, --lpos, ++it)
      {
        *it = SampleLineToString(*sit, !lpos);
      }
      return it;//empty line at end
    }

    String OrnamentToString(const VortexPlayer::Ornament& ornament);

    String PatternLineToString(const VortexPlayer::Line& line);

    template<class Iterator>
    Iterator PatternToStrings(const VortexPlayer::Pattern& pat, Iterator it)
    {
      return std::transform(pat.begin(), pat.end(), it, PatternLineToString);
    }

    template<class Iterator>
    Iterator DescriptionToStrings(const VortexDescr& descr, Iterator it)
    {
      for (std::size_t propIdx = 0; ; ++propIdx, ++it)
      {
        const String& propVal = descr.PropertyToString(propIdx);
        if (propVal.empty())
        {
          break;
        }
        *it = propVal;
      }
      return it;
    }
  }
}

#endif //__VORTEX_IO_H_DEFINED__
